#pragma once

#include "pch.h"

class CStaticPatches {
public:
    static void AgentServerCertPatch();
    static void DownloadServerCertPatch();
    static void FarmManagerCertPatch();
    static void GatewayServerCertPatch();
    static void GlobalManagerCertPatch();
    static void MachineManagerCertPatch();
    static void GameServerCertPatch();
    static void ShardManagerCertPatch();
};
