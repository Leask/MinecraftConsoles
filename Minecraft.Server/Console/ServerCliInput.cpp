#include "ServerCliInput.h"

#include "IServerCliInputSink.h"
#include "../ServerLogger.h"
#include "../vendor/linenoise/linenoise.h"
#include <lce_stdin/lce_stdin.h>
#include <lce_time/lce_time.h>
#include <lce_win32/lce_win32.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#if !defined(_WINDOWS64) && !defined(_WIN32)
#include <strings.h>
#endif

namespace
{
	bool EqualsIgnoreCase(const char *lhs, const char *rhs)
	{
		if (lhs == NULL || rhs == NULL)
		{
			return false;
		}

#if defined(_WINDOWS64) || defined(_WIN32)
		return _stricmp(lhs, rhs) == 0;
#else
		return strcasecmp(lhs, rhs) == 0;
#endif
	}

	bool UseStreamInputMode()
	{
		const char *mode = getenv("SERVER_CLI_INPUT_MODE");
		if (mode != NULL)
		{
			if (EqualsIgnoreCase(mode, "interactive")
				|| EqualsIgnoreCase(mode, "linenoise"))
			{
				return false;
			}

			return EqualsIgnoreCase(mode, "stream")
				|| EqualsIgnoreCase(mode, "stdin");
		}

#if !defined(_WINDOWS64) && !defined(_WIN32)
		return true;
#else
		return false;
#endif
	}
}

namespace ServerRuntime
{
	// C-style completion callback bridge requires a static instance pointer.
	ServerCliInput *ServerCliInput::s_instance = NULL;

	ServerCliInput::ServerCliInput()
		: m_running(false)
		, m_engine(NULL)
	{
	}

	ServerCliInput::~ServerCliInput()
	{
		Stop();
	}

	void ServerCliInput::Start(IServerCliInputSink *engine)
	{
		if (engine == NULL || m_running.exchange(true))
		{
			return;
		}

		m_engine = engine;
		s_instance = this;
		linenoiseResetStop();
		linenoiseHistorySetMaxLen(128);
		linenoiseSetCompletionCallback(&ServerCliInput::CompletionThunk);
		m_inputThread = std::thread(&ServerCliInput::RunInputLoop, this);
		LogInfo("console", "CLI input thread started.");
	}

	void ServerCliInput::Stop()
	{
		if (!m_running.exchange(false))
		{
			return;
		}

		// Ask linenoise to break out first, then join thread safely.
		linenoiseRequestStop();
		if (m_inputThread.joinable())
		{
#if defined(_WINDOWS64) || defined(_WIN32)
			CancelSynchronousIo((HANDLE)m_inputThread.native_handle());
#endif
			m_inputThread.join();
		}
		linenoiseSetCompletionCallback(NULL);

		if (s_instance == this)
		{
			s_instance = NULL;
		}

		m_engine = NULL;
		LogInfo("console", "CLI input thread stopped.");
	}

	bool ServerCliInput::IsRunning() const
	{
		return m_running.load();
	}

	void ServerCliInput::RunInputLoop()
	{
		if (UseStreamInputMode())
		{
			LogInfo("console", "CLI input mode: stream(file stdin)");
			RunStreamInputLoop();
			return;
		}

		RunLinenoiseLoop();
	}

	/**
	 *  use linenoise for interactive console input, with line editing and history support
	 */
	void ServerCliInput::RunLinenoiseLoop()
	{
		while (m_running)
		{
			char *line = linenoise("server> ");
			if (line == NULL)
			{
				// NULL is expected on stop request (or Ctrl+C inside linenoise).
				if (!m_running)
				{
					break;
				}
				LceSleepMilliseconds(10);
				continue;
			}

			EnqueueLine(line);

			linenoiseFree(line);
		}
	}

	/**
	 * use file-based stdin reading instead of linenoise when requested or when stdin is not a console/pty (e.g. piped input or non-interactive docker)
	 */
	void ServerCliInput::RunStreamInputLoop()
	{
		if (!LceStdinIsAvailable())
		{
			LogWarn("console", "stream input mode requested but STDIN handle is unavailable.");
#if defined(_WINDOWS64) || defined(_WIN32)
			LogWarn("console", "falling back to linenoise interactive mode.");
			RunLinenoiseLoop();
#endif
			return;
		}

		std::string line;
		bool skipNextLf = false;

		printf("server> ");
		fflush(stdout);

		while (m_running)
		{
			int readable = LceWaitForStdinReadable(50);
			if (readable <= 0)
			{
				LceSleepMilliseconds(10);
				continue;
			}

			char ch = 0;
			if (!LceReadStdinByte(&ch))
			{
				LceSleepMilliseconds(10);
				continue;
			}

			if (skipNextLf && ch == '\n')
			{
				skipNextLf = false;
				continue;
			}

			if (ch == '\r' || ch == '\n')
			{
				if (ch == '\r')
				{
					skipNextLf = true;
				}
				else
				{
					skipNextLf = false;
				}

				if (!line.empty())
				{
					EnqueueLine(line.c_str());
					line.clear();
				}

				printf("server> ");
				fflush(stdout);
				continue;
			}

			skipNextLf = false;

			if ((unsigned char)ch == 3)
			{
				continue;
			}

			if ((unsigned char)ch == 8 || (unsigned char)ch == 127)
			{
				if (!line.empty())
				{
					line.resize(line.size() - 1);
				}
				continue;
			}

			if (isprint((unsigned char)ch) && line.size() < 4096)
			{
				line.push_back(ch);
			}
		}
	}

	void ServerCliInput::EnqueueLine(const char *line)
	{
		if (line == NULL || line[0] == 0 || m_engine == NULL)
		{
			return;
		}

		// Keep local history and forward command for main-thread execution.
		linenoiseHistoryAdd(line);
		m_engine->EnqueueCommandLine(line);
	}

	void ServerCliInput::CompletionThunk(const char *line, linenoiseCompletions *completions)
	{
		// Static thunk forwards callback into instance state.
		if (s_instance != NULL)
		{
			s_instance->BuildCompletions(line, completions);
		}
	}

	void ServerCliInput::BuildCompletions(const char *line, linenoiseCompletions *completions)
	{
		if (line == NULL || completions == NULL || m_engine == NULL)
		{
			return;
		}

		std::vector<std::string> suggestions;
		m_engine->BuildCompletions(line, &suggestions);
		for (size_t i = 0; i < suggestions.size(); ++i)
		{
			linenoiseAddCompletion(completions, suggestions[i].c_str());
		}
	}
}
