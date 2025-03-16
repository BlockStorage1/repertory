import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart'
    show
        Validator,
        displayErrorMessage,
        getSettingDescription,
        getSettingValidators;
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
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
  Widget _createTitle(String key, String? description) {
    return Text(description == null ? key : '$key [$description]');
  }

  void _addBooleanSetting(
    list,
    root,
    key,
    value,
    isAdvanced, {
    String? description,
  }) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.switchTile(
          leading: const Icon(Icons.quiz),
          title: _createTitle(key, description),
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
    String? description,
    List<Validator> validators = const [],
  }) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          leading: const Icon(Icons.onetwothree),
          title: _createTitle(key, description),
          value: Text(value.toString()),
          onPressed: (_) {
            String updatedValue = value.toString();
            showDialog(
              context: context,
              builder: (context) {
                return AlertDialog(
                  actions: [
                    TextButton(
                      child: const Text('Cancel'),
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    TextButton(
                      child: const Text('OK'),
                      onPressed: () {
                        final result = validators.firstWhereOrNull(
                          (validator) => !validator(updatedValue),
                        );
                        if (result != null) {
                          return displayErrorMessage(
                            context,
                            "Setting '$key' is not valid",
                          );
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
                  title: _createTitle(key, description),
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
    isAdvanced, {
    String? description,
  }) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          title: _createTitle(key, description),
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

  void _addPasswordSetting(
    list,
    root,
    key,
    value,
    isAdvanced, {
    String? description,
    List<Validator> validators = const [],
  }) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          leading: const Icon(Icons.password),
          title: _createTitle(key, description),
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
                      child: const Text('Cancel'),
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    TextButton(
                      child: const Text('OK'),
                      onPressed: () {
                        if (updatedValue1 != updatedValue2) {
                          return displayErrorMessage(
                            context,
                            "Setting '$key' does not match",
                          );
                        }

                        final result = validators.firstWhereOrNull(
                          (validator) => !validator(updatedValue1),
                        );
                        if (result != null) {
                          return displayErrorMessage(
                            context,
                            "Setting '$key' is not valid",
                          );
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
                      const SizedBox(height: constants.padding),
                      TextField(
                        autofocus: false,
                        controller: TextEditingController(text: updatedValue2),
                        obscureText: true,
                        obscuringCharacter: '*',
                        onChanged: (value) => updatedValue2 = value,
                      ),
                    ],
                  ),
                  title: _createTitle(key, description),
                );
              },
            );
          },
        ),
      );
    }
  }

  void _addStringListSetting(
    list,
    root,
    key,
    value,
    List<String> valueList,
    icon,
    isAdvanced, {
    String? description,
  }) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          title: _createTitle(key, description),
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

  void _addStringSetting(
    list,
    root,
    key,
    value,
    icon,
    isAdvanced, {
    String? description,
    List<Validator> validators = const [],
  }) {
    if (!isAdvanced || widget.showAdvanced) {
      list.add(
        SettingsTile.navigation(
          leading: Icon(icon),
          title: _createTitle(key, description),
          value: Text(value),
          onPressed: (_) {
            String updatedValue = value;
            showDialog(
              context: context,
              builder: (context) {
                return AlertDialog(
                  actions: [
                    TextButton(
                      child: const Text('Cancel'),
                      onPressed: () => Navigator.of(context).pop(),
                    ),
                    TextButton(
                      child: const Text('OK'),
                      onPressed: () {
                        final result = validators.firstWhereOrNull(
                          (validator) => !validator(updatedValue),
                        );
                        if (result != null) {
                          return displayErrorMessage(
                            context,
                            "Setting '$key' is not valid",
                          );
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
                  title: _createTitle(key, description),
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
        case 'ApiPassword':
          {
            _addPasswordSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'DatabaseType':
          {
            _addStringListSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              constants.databaseTypeList,
              Icons.dataset,
              true,
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
            );
          }
          break;
        case 'EventLevel':
          {
            _addStringListSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              constants.eventLevelList,
              Icons.event,
              false,
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'PreferredDownloadType':
          {
            _addStringListSetting(
              commonSettings,
              widget.settings,
              key,
              value,
              constants.downloadTypeList,
              Icons.download,
              false,
              description: getSettingDescription(key),
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
              description: getSettingDescription(key),
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
              constants.ringBufferSizeList,
              512,
              Icons.animation,
              false,
              description: getSettingDescription(key),
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
                  description: getSettingDescription('$key.$subKey'),
                  validators: [
                    ...getSettingValidators('$key.$subKey'),
                    (value) =>
                        !Provider.of<MountList>(
                          context,
                          listen: false,
                        ).hasBucketName(
                          widget.mount.type,
                          value,
                          excludeName: widget.mount.name,
                        ),
                  ],
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Protocol':
          {
            _addStringListSetting(
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              constants.protocolTypeList,
              Icons.http,
              true,
              description: getSettingDescription(key),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
              validators: [
                ...getSettingValidators('$key.$subKey'),
                (value) =>
                    !Provider.of<MountList>(
                      context,
                      listen: false,
                    ).hasBucketName(
                      widget.mount.type,
                      value,
                      excludeName: widget.mount.name,
                    ),
              ],
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
            );
            remoteMountSettings.insertAll(0, tempSettings);
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
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
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
      }
    });
  }
}
