import 'dart:convert';

import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:repertory/models/mount.dart';
import 'package:settings_ui/settings_ui.dart';

class MountSettingsWidget extends StatefulWidget {
  final Mount mount;
  const MountSettingsWidget({super.key, required this.mount});

  @override
  State<MountSettingsWidget> createState() => _MountSettingsWidgetState();
}

class _MountSettingsWidgetState extends State<MountSettingsWidget> {
  Map<String, dynamic> _settings = {};

  void _addBooleanSetting(list, root, key, value) {
    list.add(
      SettingsTile.switchTile(
        leading: Icon(Icons.quiz),
        title: Text(key),
        initialValue: (value as bool),
        onPressed: (_) {
          setState(() {
            root[key] = !value;
          });
        },
        onToggle: (bool nextValue) {
          setState(() {
            root[key] = nextValue;
          });
        },
      ),
    );
  }

  void _addIntSetting(list, root, key, value) {
    list.add(
      SettingsTile.navigation(
        leading: Icon(Icons.onetwothree),
        title: Text(key),
        value: Text(value.toString()),
        onPressed: (_) {
          String updatedValue = value.toString();
          showDialog(
            context: context,
            builder: (context) {
              return AlertDialog(
                actions: [
                  TextButton(
                    child: Text('Cancel'),
                    onPressed: () {
                      Navigator.of(context).pop();
                    },
                  ),
                  TextButton(
                    child: Text('OK'),
                    onPressed: () {
                      setState(() {
                        root[key] = int.parse(updatedValue);
                      });
                      Navigator.of(context).pop();
                    },
                  ),
                ],
                content: TextField(
                  autofocus: true,
                  controller: TextEditingController(text: updatedValue),
                  inputFormatters: [FilteringTextInputFormatter.digitsOnly],
                  keyboardType: TextInputType.number,
                  onChanged: (nextValue) {
                    updatedValue = nextValue;
                  },
                ),
                title: Text(key),
              );
            },
          );
        },
      ),
    );
  }

  void _addIntListSetting(
    list,
    root,
    key,
    value,
    List<String> valueList,
    defaultValue,
    icon,
  ) {
    list.add(
      SettingsTile.navigation(
        title: Text(key),
        leading: Icon(icon),
        value: DropdownButton<String>(
          value: value.toString(),
          onChanged: (newValue) {
            setState(() {
              root[key] = int.parse(newValue ?? defaultValue.toString());
            });
          },
          items:
              valueList.map<DropdownMenuItem<String>>((item) {
                return DropdownMenuItem<String>(value: item, child: Text(item));
              }).toList(),
        ),
      ),
    );
  }

  void _addListSetting(list, root, key, value, List<String> valueList, icon) {
    list.add(
      SettingsTile.navigation(
        title: Text(key),
        leading: Icon(icon),
        value: DropdownButton<String>(
          value: value,
          onChanged: (newValue) {
            setState(() {
              root[key] = newValue;
            });
          },
          items:
              valueList.map<DropdownMenuItem<String>>((item) {
                return DropdownMenuItem<String>(value: item, child: Text(item));
              }).toList(),
        ),
      ),
    );
  }

  void _addPasswordSetting(list, root, key, value) {
    list.add(
      SettingsTile.navigation(
        leading: Icon(Icons.password),
        title: Text(key),
        value: Text('*' * (value as String).length),
      ),
    );
  }

  void _addStringSetting(list, root, key, value, icon) {
    list.add(
      SettingsTile.navigation(
        leading: Icon(icon),
        title: Text(key),
        value: Text(value),
        onPressed: (_) {
          String updatedValue = value;
          showDialog(
            context: context,
            builder: (context) {
              return AlertDialog(
                actions: [
                  TextButton(
                    child: Text('Cancel'),
                    onPressed: () {
                      Navigator.of(context).pop();
                    },
                  ),
                  TextButton(
                    child: Text('OK'),
                    onPressed: () {
                      setState(() {
                        root[key] = updatedValue;
                      });
                      Navigator.of(context).pop();
                    },
                  ),
                ],
                content: TextField(
                  autofocus: true,
                  controller: TextEditingController(text: updatedValue),
                  onChanged: (value) {
                    updatedValue = value;
                  },
                ),
                title: Text(key),
              );
            },
          );
        },
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    List<SettingsTile> commonSettings = [];
    List<SettingsTile> encryptConfigSettings = [];
    List<SettingsTile> hostConfigSettings = [];
    List<SettingsTile> remoteConfigSettings = [];
    List<SettingsTile> remoteMountSettings = [];
    List<SettingsTile> s3ConfigSettings = [];
    List<SettingsTile> siaConfigSettings = [];

    _settings.forEach((key, value) {
      if (key == "ApiAuth") {
        _addPasswordSetting(commonSettings, _settings, key, value);
      } else if (key == "ApiPort") {
        _addIntSetting(commonSettings, _settings, key, value);
      } else if (key == "ApiUser") {
        _addStringSetting(commonSettings, _settings, key, value, Icons.person);
      } else if (key == "DatabaseType") {
        _addListSetting(commonSettings, _settings, key, value, [
          "rocksdb",
          "sqlite",
        ], Icons.dataset);
      } else if (key == "DownloadTimeoutSeconds") {
        _addIntSetting(commonSettings, _settings, key, value);
      } else if (key == "EnableDownloadTimeout") {
        _addBooleanSetting(commonSettings, _settings, key, value);
      } else if (key == "EnableDriveEvents") {
        _addBooleanSetting(commonSettings, _settings, key, value);
      } else if (key == "EventLevel") {
        _addListSetting(commonSettings, _settings, key, value, [
          "critical",
          "error",
          "warn",
          "info",
          "debug",
          "trace",
        ], Icons.event);
      } else if (key == "EvictionDelayMinutes") {
        _addIntSetting(commonSettings, _settings, key, value);
      } else if (key == "EvictionUseAccessedTime") {
        _addBooleanSetting(commonSettings, _settings, key, value);
      } else if (key == "MaxCacheSizeBytes") {
        _addIntSetting(commonSettings, _settings, key, value);
      } else if (key == "MaxUploadCount") {
        _addIntSetting(commonSettings, _settings, key, value);
      } else if (key == "OnlineCheckRetrySeconds") {
        _addIntSetting(commonSettings, _settings, key, value);
      } else if (key == "PreferredDownloadType") {
        _addListSetting(commonSettings, _settings, key, value, [
          "default",
          "direct",
          "ring_buffer",
        ], Icons.download);
      } else if (key == "RetryReadCount") {
        _addIntSetting(commonSettings, _settings, key, value);
      } else if (key == "RingBufferFileSize") {
        _addIntListSetting(
          commonSettings,
          _settings,
          key,
          value,
          ["128", "256", "512", "1024", "2048"],
          512,
          Icons.animation,
        );
      } else if (key == "EncryptConfig") {
        value.forEach((subKey, subValue) {
          if (subKey == "EncryptionToken") {
            _addPasswordSetting(
              encryptConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "Path") {
            _addStringSetting(
              encryptConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.folder,
            );
          }
        });
      } else if (key == "HostConfig") {
        value.forEach((subKey, subValue) {
          if (subKey == "AgentString") {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.support_agent,
            );
          } else if (subKey == "ApiPassword") {
            _addPasswordSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "ApiPort") {
            _addIntSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "ApiUser") {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.person,
            );
          } else if (subKey == "HostNameOrIp") {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.computer,
            );
          } else if (subKey == "Path") {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.route,
            );
          } else if (subKey == "Protocol") {
            _addListSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              ["http", "https"],
              Icons.http,
            );
          } else if (subKey == "TimeoutMs") {
            _addIntSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          }
        });
      } else if (key == "RemoteConfig") {
        value.forEach((subKey, subValue) {
          if (subKey == "ApiPort") {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "EncryptionToken") {
            _addPasswordSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "HostNameOrIp") {
            _addStringSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.computer,
            );
          } else if (subKey == "MaxConnections") {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "ReceiveTimeoutMs") {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "SendTimeoutMs") {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          }
        });
      } else if (key == "RemoteMount") {
        value.forEach((subKey, subValue) {
          if (subKey == "Enable") {
            List<SettingsTile> tempSettings = [];
            _addBooleanSetting(tempSettings, _settings[key], subKey, subValue);
            remoteMountSettings.insertAll(0, tempSettings);
          } else if (subKey == "ApiPort") {
            _addIntSetting(
              remoteMountSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "ClientPoolSize") {
            _addIntSetting(
              remoteMountSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "EncryptionToken") {
            _addPasswordSetting(
              remoteMountSettings,
              _settings[key],
              subKey,
              subValue,
            );
          }
        });
      } else if (key == "S3Config") {
        value.forEach((subKey, subValue) {
          if (subKey == "AccessKey") {
            _addPasswordSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "Bucket") {
            _addStringSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.folder,
            );
          } else if (subKey == "EncryptionToken") {
            _addPasswordSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "Region") {
            _addStringSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.map,
            );
          } else if (subKey == "SecretKey") {
            _addPasswordSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "TimeoutMs") {
            _addIntSetting(s3ConfigSettings, _settings[key], subKey, subValue);
          } else if (subKey == "URL") {
            _addStringSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.http,
            );
          } else if (subKey == "UsePathStyle") {
            _addBooleanSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          } else if (subKey == "UseRegionInURL") {
            _addBooleanSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
            );
          }
        });
      } else if (key == "SiaConfig") {
        value.forEach((subKey, subValue) {
          if (subKey == "Bucket") {
            _addStringSetting(
              siaConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.folder,
            );
          }
        });
      }
    });

    return SettingsList(
      shrinkWrap: false,
      sections: [
        if (encryptConfigSettings.isNotEmpty)
          SettingsSection(
            title: const Text('Encrypt Config'),
            tiles: encryptConfigSettings,
          ),
        if (hostConfigSettings.isNotEmpty)
          SettingsSection(
            title: const Text('Host Config'),
            tiles: hostConfigSettings,
          ),
        if (remoteConfigSettings.isNotEmpty)
          SettingsSection(
            title: const Text('Remote Config'),
            tiles: remoteConfigSettings,
          ),
        if (s3ConfigSettings.isNotEmpty)
          SettingsSection(
            title: const Text('S3 Config'),
            tiles: s3ConfigSettings,
          ),
        if (siaConfigSettings.isNotEmpty)
          SettingsSection(
            title: const Text('Sia Config'),
            tiles: siaConfigSettings,
          ),
        if (remoteMountSettings.isNotEmpty)
          SettingsSection(
            title: const Text('Remote Mount'),
            tiles:
                _settings["RemoteMount"]["Enable"] as bool
                    ? remoteMountSettings
                    : [remoteMountSettings[0]],
          ),
        SettingsSection(title: const Text('Settings'), tiles: commonSettings),
      ],
    );
  }

  @override
  void dispose() {
    var settings = widget.mount.mountConfig.settings;
    if (!DeepCollectionEquality().equals(_settings, settings)) {
      _settings.forEach((key, value) {
        if (!DeepCollectionEquality().equals(settings[key], value)) {
          if (value is Map<String, dynamic>) {
            value.forEach((subKey, subValue) {
              if (!DeepCollectionEquality().equals(
                settings[key][subKey],
                subValue,
              )) {
                widget.mount.setValue('$key.$subKey', subValue.toString());
              }
            });
          } else {
            widget.mount.setValue(key, value.toString());
          }
        }
      });
    }
    super.dispose();
  }

  @override
  void initState() {
    _settings = jsonDecode(jsonEncode(widget.mount.mountConfig.settings));
    super.initState();
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
