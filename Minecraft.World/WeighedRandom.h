#pragma once
// 4J - this WeighedRandomItem class was a nested static class within WeighedRandom, but we need to be able to refer to it externally

class WeighedRandomItem
{
	friend class WeighedRandom;
protected:
	int randomWeight;

public:
	WeighedRandomItem(int randomWeight)
	{
        this->randomWeight = randomWeight;
    }
};

class WeighedRandom
{
public:
	// 4J - vectors here were Collection<? extends WeighedRandomItem>
	static int getTotalWeight(vector<WeighedRandomItem *> *items);	
	static WeighedRandomItem *getRandomItem(Random *random, vector<WeighedRandomItem *> *items, int totalWeight);
	static WeighedRandomItem *getRandomItem(Random *random, vector<WeighedRandomItem *> *items);
	template <typename TItem>
	static int getTotalWeight(vector<TItem *> *items)
	{
		int totalWeight = 0;
		for (const auto &item : *items)
		{
			totalWeight +=
				static_cast<WeighedRandomItem *>(item)->randomWeight;
		}
		return totalWeight;
	}

	template <typename TItem>
	static TItem *getRandomItem(Random *random, vector<TItem *> *items,
		int totalWeight)
	{
		if (totalWeight <= 0)
		{
			__debugbreak();
		}

		int selection = random->nextInt(totalWeight);
		for (const auto &item : *items)
		{
			selection -=
				static_cast<WeighedRandomItem *>(item)->randomWeight;
			if (selection < 0)
			{
				return item;
			}
		}
		return nullptr;
	}

	template <typename TItem>
	static TItem *getRandomItem(Random *random, vector<TItem *> *items)
	{
		return getRandomItem(random, items, getTotalWeight(items));
	}
	static int getTotalWeight(WeighedRandomItemArray items);
	static WeighedRandomItem *getRandomItem(Random *random, WeighedRandomItemArray items, int totalWeight);
	static WeighedRandomItem *getRandomItem(Random *random, WeighedRandomItemArray items);

};
