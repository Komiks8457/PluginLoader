# PluginLoader
Plugin Library Loader

What does PluginLoader do?
* already contains the certification patch of each modules.
* ability to also load your custom dll for each modules.
* automatically load vanguard dll's if exists. (read REMINDER)
* ability to hide/show vanguard console window.

Loading your custom dll:
* Make a Plugin folder and place your dll, your dll name should be the same as your target module.
  E.g. MachineManager_patch1.dll, MachineManager_patch2.dll it will be loaded for MachineManager.exe or SR_GameServer.dll will be loaded for SR_GameServer.exe.
  please note that dll will be load in alphabetical order, for example above MachineManager_patch1.dll will be loaded first.

Hide Console window of Vanguard
* Simply add HideConsole key with value TRUE or FALSE on each .ini of vanguard.
  E.g. GatewayServer.ini
  ```ini
  [GatewayServer]
  Key=
  PacketDebugging=0
  HideConsole=true
  ConnectionString=
  ```

REMINDER
* In order to load Vanguard, the dll should be inside Vanguard folder together with its ini files.
* Vanguard is attached to this modules (AgentServer, DownloadServer, GatewayServer, SR_GameServer, SR_ShardManager).
  if ever you have a custom hooks on these mentioned modules, ask bim first for the address you hooking to, if bim say's he is hooking on it then you should find another way to hook your own stuff. Avoid double hooking!
