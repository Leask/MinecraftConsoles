#pragma once

struct ServerPropertiesSmokeResult
{
    bool createdDefaults;
    bool normalizedValues;
    bool savePersisted;
    bool whitelistWorkflowOk;
};

ServerPropertiesSmokeResult RunServerPropertiesSmoke();
