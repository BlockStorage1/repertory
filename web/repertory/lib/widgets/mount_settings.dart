import 'dart:convert';

import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:repertory/constants.dart';
import 'package:repertory/models/mount.dart';
import 'package:settings_ui/settings_ui.dart';

class MountSettingsWidget extends StatefulWidget {
  final bool isAdd;
  final bool showAdvanced;
  final Mount mount;
  final Function? onChanged;
  const MountSettingsWidget({
    super.key,
    this.isAdd = false,
    required this.mount,
    this.onChanged,
    required this.showAdvanced,
  });

  @override
  State<MountSettingsWidget> createState() => _MountSettingsWidgetState();
}

class _MountSettingsWidgetState extends State<MountSettingsWidget> {
  Map<String, dynamic> _settings = {};

  void _addBooleanSetting(list, root, key, value, isAdvanced) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.switchTile(
          leading: Icon(Icons.quiz),
          title: Text(key),
          initialValue: (value as bool),
          onPressed: (_) {
            setState(() {
              root[key] = !value;
              widget.onChanged?.call(_settings);
            });
          },
          onToggle: (bool nextValue) {
            setState(() {
              root[key] = nextValue;
              widget.onChanged?.call(_settings);
            });
          },
        ),
      );
    }
  }

  void _addIntSetting(list, root, key, value, isAdvanced) {
    if (!isAdvanced || widget.showAdvanced) {
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
                          widget.onChanged?.call(_settings);
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
  }

  void _addIntListSetting(
    list,
    root,
    key,
    value,
    List<String> valueList,
    defaultValue,
    icon,
    isAdvanced,
  ) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          title: Text(key),
          leading: Icon(icon),
          value: DropdownButton<String>(
            value: value.toString(),
            onChanged: (newValue) {
              setState(() {
                root[key] = int.parse(newValue ?? defaultValue.toString());
                widget.onChanged?.call(_settings);
              });
            },
            items:
                valueList.map<DropdownMenuItem<String>>((item) {
                  return DropdownMenuItem<String>(
                    value: item,
                    child: Text(item),
                  );
                }).toList(),
          ),
        ),
      );
    }
  }

  void _addListSetting(
    list,
    root,
    key,
    value,
    List<String> valueList,
    icon,
    isAdvanced,
  ) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          title: Text(key),
          leading: Icon(icon),
          value: DropdownButton<String>(
            value: value,
            onChanged: (newValue) {
              setState(() {
                root[key] = newValue;
                widget.onChanged?.call(_settings);
              });
            },
            items:
                valueList.map<DropdownMenuItem<String>>((item) {
                  return DropdownMenuItem<String>(
                    value: item,
                    child: Text(item),
                  );
                }).toList(),
          ),
        ),
      );
    }
  }

  void _addPasswordSetting(list, root, key, value, isAdvanced) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          leading: Icon(Icons.password),
          title: Text(key),
          value: Text('*' * (value as String).length),
        ),
      );
    }
  }

  void _addStringSetting(list, root, key, value, icon, isAdvanced) {
    if (!isAdvanced || widget.showAdvanced) {
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
                          widget.onChanged?.call(_settings);
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
      if (key == 'ApiAuth') {
        _addPasswordSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'ApiPort') {
        _addIntSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'ApiUser') {
        _addStringSetting(
          commonSettings,
          _settings,
          key,
          value,
          Icons.person,
          true,
        );
      } else if (key == 'DatabaseType') {
        _addListSetting(
          commonSettings,
          _settings,
          key,
          value,
          databaseTypeList,
          Icons.dataset,
          true,
        );
      } else if (key == 'DownloadTimeoutSeconds') {
        _addIntSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'EnableDownloadTimeout') {
        _addBooleanSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'EnableDriveEvents') {
        _addBooleanSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'EventLevel') {
        _addListSetting(
          commonSettings,
          _settings,
          key,
          value,
          eventLevelList,
          Icons.event,
          false,
        );
      } else if (key == 'EvictionDelayMinutes') {
        _addIntSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'EvictionUseAccessedTime') {
        _addBooleanSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'MaxCacheSizeBytes') {
        _addIntSetting(commonSettings, _settings, key, value, false);
      } else if (key == 'MaxUploadCount') {
        _addIntSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'OnlineCheckRetrySeconds') {
        _addIntSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'PreferredDownloadType') {
        _addListSetting(
          commonSettings,
          _settings,
          key,
          value,
          downloadTypeList,
          Icons.download,
          false,
        );
      } else if (key == 'RetryReadCount') {
        _addIntSetting(commonSettings, _settings, key, value, true);
      } else if (key == 'RingBufferFileSize') {
        _addIntListSetting(
          commonSettings,
          _settings,
          key,
          value,
          ringBufferSizeList,
          512,
          Icons.animation,
          false,
        );
      } else if (key == 'EncryptConfig') {
        value.forEach((subKey, subValue) {
          if (subKey == 'EncryptionToken') {
            _addPasswordSetting(
              encryptConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'Path') {
            _addStringSetting(
              encryptConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.folder,
              false,
            );
          }
        });
      } else if (key == 'HostConfig') {
        value.forEach((subKey, subValue) {
          if (subKey == 'AgentString') {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.support_agent,
              true,
            );
          } else if (subKey == 'ApiPassword') {
            _addPasswordSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'ApiPort') {
            _addIntSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'ApiUser') {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.person,
              true,
            );
          } else if (subKey == 'HostNameOrIp') {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.computer,
              false,
            );
          } else if (subKey == 'Path') {
            _addStringSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.route,
              true,
            );
          } else if (subKey == 'Protocol') {
            _addListSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              protocolTypeList,
              Icons.http,
              true,
            );
          } else if (subKey == 'TimeoutMs') {
            _addIntSetting(
              hostConfigSettings,
              _settings[key],
              subKey,
              subValue,
              true,
            );
          }
        });
      } else if (key == 'RemoteConfig') {
        value.forEach((subKey, subValue) {
          if (subKey == 'ApiPort') {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'EncryptionToken') {
            _addPasswordSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'HostNameOrIp') {
            _addStringSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.computer,
              false,
            );
          } else if (subKey == 'MaxConnections') {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
              true,
            );
          } else if (subKey == 'ReceiveTimeoutMs') {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
              true,
            );
          } else if (subKey == 'SendTimeoutMs') {
            _addIntSetting(
              remoteConfigSettings,
              _settings[key],
              subKey,
              subValue,
              true,
            );
          }
        });
      } else if (key == 'RemoteMount') {
        value.forEach((subKey, subValue) {
          if (subKey == 'Enable') {
            List<SettingsTile> tempSettings = [];
            _addBooleanSetting(
              tempSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
            remoteMountSettings.insertAll(0, tempSettings);
          } else if (subKey == 'ApiPort') {
            _addIntSetting(
              remoteMountSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'ClientPoolSize') {
            _addIntSetting(
              remoteMountSettings,
              _settings[key],
              subKey,
              subValue,
              true,
            );
          } else if (subKey == 'EncryptionToken') {
            _addPasswordSetting(
              remoteMountSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          }
        });
      } else if (key == 'S3Config') {
        value.forEach((subKey, subValue) {
          if (subKey == 'AccessKey') {
            _addPasswordSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'Bucket') {
            _addStringSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.folder,
              false,
            );
          } else if (subKey == 'EncryptionToken') {
            _addPasswordSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'Region') {
            _addStringSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.map,
              false,
            );
          } else if (subKey == 'SecretKey') {
            _addPasswordSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'TimeoutMs') {
            _addIntSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              true,
            );
          } else if (subKey == 'URL') {
            _addStringSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.http,
              false,
            );
          } else if (subKey == 'UsePathStyle') {
            _addBooleanSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          } else if (subKey == 'UseRegionInURL') {
            _addBooleanSetting(
              s3ConfigSettings,
              _settings[key],
              subKey,
              subValue,
              false,
            );
          }
        });
      } else if (key == 'SiaConfig') {
        value.forEach((subKey, subValue) {
          if (subKey == 'Bucket') {
            _addStringSetting(
              siaConfigSettings,
              _settings[key],
              subKey,
              subValue,
              Icons.folder,
              false,
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
                _settings['RemoteMount']['Enable'] as bool
                    ? remoteMountSettings
                    : [remoteMountSettings[0]],
          ),
        SettingsSection(title: const Text('Settings'), tiles: commonSettings),
      ],
    );
  }

  @override
  void dispose() {
    if (!widget.isAdd) {
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
