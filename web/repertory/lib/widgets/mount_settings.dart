import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:repertory/constants.dart';
import 'package:repertory/helpers.dart' show Validator, getSettingValidators;
import 'package:repertory/models/mount.dart';
import 'package:settings_ui/settings_ui.dart';

class MountSettingsWidget extends StatefulWidget {
  final bool isAdd;
  final bool showAdvanced;
  final Mount mount;
  final Function? onChanged;
  final Map<String, dynamic> settings;
  const MountSettingsWidget({
    super.key,
    this.isAdd = false,
    required this.mount,
    this.onChanged,
    required this.settings,
    required this.showAdvanced,
  });

  @override
  State<MountSettingsWidget> createState() => _MountSettingsWidgetState();
}

class _MountSettingsWidgetState extends State<MountSettingsWidget> {
  static const _padding = 15.0;

  void _addBooleanSetting(list, root, key, value, isAdvanced) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.switchTile(
          leading: Icon(Icons.quiz),
          title: Text(key),
          initialValue: (value as bool),
          onPressed:
              (_) => setState(() {
                root[key] = !value;
                widget.onChanged?.call(widget.settings);
              }),
          onToggle: (bool nextValue) {
            setState(() {
              root[key] = nextValue;
              widget.onChanged?.call(widget.settings);
            });
          },
        ),
      );
    }
  }

  void _addIntSetting(
    list,
    root,
    key,
    value,
    isAdvanced, {
    List<Validator> validators = const [],
  }) {
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
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    TextButton(
                      child: Text('OK'),
                      onPressed: () {
                        final result = validators.firstWhereOrNull(
                          (validator) => !validator(updatedValue),
                        );
                        if (result != null) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(
                              content: Text(
                                "'$key' is not valid",
                                textAlign: TextAlign.center,
                              ),
                            ),
                          );
                          return;
                        }
                        setState(() {
                          root[key] = int.parse(updatedValue);
                          widget.onChanged?.call(widget.settings);
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
                    onChanged: (nextValue) => updatedValue = nextValue,
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
                widget.onChanged?.call(widget.settings);
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
            onChanged:
                (newValue) => setState(() {
                  root[key] = newValue;
                  widget.onChanged?.call(widget.settings);
                }),
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

  void _addPasswordSetting(
    list,
    root,
    key,
    value,
    isAdvanced, {
    List<Validator> validators = const [],
  }) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          leading: Icon(Icons.password),
          title: Text(key),
          value: Text('*' * (value as String).length),
          onPressed: (_) {
            String updatedValue1 = value;
            String updatedValue2 = value;
            showDialog(
              context: context,
              builder: (context) {
                return AlertDialog(
                  actions: [
                    TextButton(
                      child: Text('Cancel'),
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    TextButton(
                      child: Text('OK'),
                      onPressed: () {
                        if (updatedValue1 != updatedValue2) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(
                              content: Text(
                                "'$key' does not match",
                                textAlign: TextAlign.center,
                              ),
                            ),
                          );
                          return;
                        }

                        final result = validators.firstWhereOrNull(
                          (validator) => !validator(updatedValue1),
                        );
                        if (result != null) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(
                              content: Text(
                                "'$key' is not valid",
                                textAlign: TextAlign.center,
                              ),
                            ),
                          );
                          return;
                        }

                        setState(() {
                          root[key] = updatedValue1;
                          widget.onChanged?.call(widget.settings);
                        });
                        Navigator.of(context).pop();
                      },
                    ),
                  ],
                  content: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisAlignment: MainAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      TextField(
                        autofocus: true,
                        controller: TextEditingController(text: updatedValue1),
                        obscureText: true,
                        obscuringCharacter: '*',
                        onChanged: (value) => updatedValue1 = value,
                      ),
                      const SizedBox(height: _padding),
                      TextField(
                        autofocus: false,
                        controller: TextEditingController(text: updatedValue2),
                        obscureText: true,
                        obscuringCharacter: '*',
                        onChanged: (value) => updatedValue2 = value,
                      ),
                    ],
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

  void _addStringSetting(
    list,
    root,
    key,
    value,
    icon,
    isAdvanced, {
    List<Validator> validators = const [],
  }) {
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
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    TextButton(
                      child: Text('OK'),
                      onPressed: () {
                        final result = validators.firstWhereOrNull(
                          (validator) => !validator(updatedValue),
                        );
                        if (result != null) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(
                              content: Text(
                                "'$key' is not valid",
                                textAlign: TextAlign.center,
                              ),
                            ),
                          );
                          return;
                        }
                        setState(() {
                          root[key] = updatedValue;
                          widget.onChanged?.call(widget.settings);
                        });
                        Navigator.of(context).pop();
                      },
                    ),
                  ],
                  content: TextField(
                    autofocus: true,
                    controller: TextEditingController(text: updatedValue),
                    inputFormatters: [
                      FilteringTextInputFormatter.deny(RegExp(r'\s')),
                    ],
                    onChanged: (value) => updatedValue = value,
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

    widget.settings.forEach((key, value) {
      switch (key) {
        case 'ApiAuth':
          {
            _addPasswordSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'ApiPort':
          {
            _addIntSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'ApiUser':
          {
            _addStringSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              Icons.person,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'DatabaseType':
          {
            _addListSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              databaseTypeList,
              Icons.dataset,
              true,
            );
          }
          break;
        case 'DownloadTimeoutSeconds':
          {
            _addIntSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'EnableDownloadTimeout':
          {
            _addBooleanSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
            );
          }
          break;
        case 'EnableDriveEvents':
          {
            _addBooleanSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
            );
          }
          break;
        case 'EventLevel':
          {
            _addListSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              eventLevelList,
              Icons.event,
              false,
            );
          }
          break;
        case 'EvictionDelayMinutes':
          {
            _addIntSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'EvictionUseAccessedTime':
          {
            _addBooleanSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
            );
          }
          break;
        case 'MaxCacheSizeBytes':
          {
            _addIntSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              false,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'MaxUploadCount':
          {
            _addIntSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'OnlineCheckRetrySeconds':
          {
            _addIntSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'PreferredDownloadType':
          {
            _addListSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              downloadTypeList,
              Icons.download,
              false,
            );
          }
          break;
        case 'RetryReadCount':
          {
            _addIntSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'RingBufferFileSize':
          {
            _addIntListSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              ringBufferSizeList,
              512,
              Icons.animation,
              false,
            );
          }
          break;
        case 'EncryptConfig':
          {
            _parseEncryptConfig(encryptConfigSettings, key, value);
          }
          break;
        case 'HostConfig':
          {
            _parseHostConfig(hostConfigSettings, key, value);
          }
          break;
        case 'RemoteConfig':
          {
            _parseRemoteConfig(remoteConfigSettings, key, value);
          }
          break;
        case 'RemoteMount':
          {
            _parseRemoteMount(remoteMountSettings, key, value);
          }
          break;
        case 'S3Config':
          {
            _parseS3Config(s3ConfigSettings, key, value);
          }
          break;
        case 'SiaConfig':
          {
            value.forEach((subKey, subValue) {
              if (subKey == 'Bucket') {
                _addStringSetting(
                  siaConfigSettings,
                  widget.settings[key],
                  subKey,
                  subValue,
                  Icons.folder,
                  false,
                  validators: getSettingValidators('$key.$subKey'),
                );
              }
            });
          }
          break;
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
                widget.settings['RemoteMount']['Enable'] as bool
                    ? remoteMountSettings
                    : [remoteMountSettings[0]],
          ),
        if (commonSettings.isNotEmpty)
          SettingsSection(title: const Text('Settings'), tiles: commonSettings),
      ],
    );
  }

  void _parseHostConfig(List<SettingsTile> hostConfigSettings, key, value) {
    value.forEach((subKey, subValue) {
      switch (subKey) {
        case 'AgentString':
          {
            _addStringSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.support_agent,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ApiPassword':
          {
            _addPasswordSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ApiPort':
          {
            _addIntSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ApiUser':
          {
            _addStringSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.person,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'HostNameOrIp':
          {
            _addStringSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.computer,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Path':
          {
            _addStringSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.route,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Protocol':
          {
            _addListSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              protocolTypeList,
              Icons.http,
              true,
            );
          }
          break;
        case 'TimeoutMs':
          {
            _addIntSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
      }
    });
  }

  void _parseEncryptConfig(
    List<SettingsTile> encryptConfigSettings,
    key,
    value,
  ) {
    value.forEach((subKey, subValue) {
      switch (subKey) {
        case 'EncryptionToken':
          {
            _addPasswordSetting(
              encryptConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Path':
          {
            _addStringSetting(
              encryptConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.folder,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
      }
    });
  }

  @override
  void dispose() {
    if (!widget.isAdd) {
      var settings = widget.mount.mountConfig.settings;
      if (!DeepCollectionEquality().equals(widget.settings, settings)) {
        widget.settings.forEach((key, value) {
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

  void _parseS3Config(List<SettingsTile> s3ConfigSettings, String key, value) {
    value.forEach((subKey, subValue) {
      switch (subKey) {
        case 'AccessKey':
          {
            _addStringSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.key,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Bucket':
          {
            _addStringSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.folder,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'EncryptionToken':
          {
            _addPasswordSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Region':
          {
            _addStringSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.map,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'SecretKey':
          {
            _addPasswordSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'TimeoutMs':
          {
            _addIntSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'URL':
          {
            _addStringSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.http,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'UsePathStyle':
          {
            _addBooleanSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
            );
          }
          break;
        case 'UseRegionInURL':
          {
            _addBooleanSetting(
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
            );
          }
          break;
      }
    });
  }

  void _parseRemoteMount(
    List<SettingsTile> remoteMountSettings,
    String key,
    value,
  ) {
    value.forEach((subKey, subValue) {
      switch (subKey) {
        case 'Enable':
          {
            List<SettingsTile> tempSettings = [];
            _addBooleanSetting(
              tempSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
            );
            remoteMountSettings.insertAll(1, tempSettings);
          }
          break;
        case 'ApiPort':
          {
            _addIntSetting(
              remoteMountSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ClientPoolSize':
          {
            _addIntSetting(
              remoteMountSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'EncryptionToken':
          {
            _addPasswordSetting(
              remoteMountSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
      }
    });
  }

  void _parseRemoteConfig(
    List<SettingsTile> remoteConfigSettings,
    String key,
    value,
  ) {
    value.forEach((subKey, subValue) {
      switch (subKey) {
        case 'ApiPort':
          {
            _addIntSetting(
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'EncryptionToken':
          {
            _addPasswordSetting(
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'HostNameOrIp':
          {
            _addStringSetting(
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.computer,
              false,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'MaxConnections':
          {
            _addIntSetting(
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ReceiveTimeoutMs':
          {
            _addIntSetting(
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'SendTimeoutMs':
          {
            _addIntSetting(
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
      }
    });
  }
}
