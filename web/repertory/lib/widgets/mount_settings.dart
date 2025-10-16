// mount_settings.dart

import 'package:flutter/material.dart';
import 'package:flutter_settings_ui/flutter_settings_ui.dart'
    show SettingsTile, SettingsList, DevicePlatform;
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart'
    show
        convertAllToString,
        getChanged,
        getSettingDescription,
        getSettingValidators,
        createSettingsTheme;
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/settings.dart';
import 'package:repertory/widgets/settings/settings_section.dart';

class MountSettingsWidget extends StatefulWidget {
  final bool isAdd;
  final bool showAdvanced;
  final Mount mount;
  final Map<String, dynamic> settings;
  final Function? onChanged;
  const MountSettingsWidget({
    super.key,
    this.isAdd = false,
    this.onChanged,
    required this.mount,
    required this.settings,
    required this.showAdvanced,
  });

  @override
  State<MountSettingsWidget> createState() => _MountSettingsWidgetState();
}

class _MountSettingsWidgetState extends State<MountSettingsWidget> {
  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final scheme = theme.colorScheme;
    final settingsTheme = createSettingsTheme(scheme);

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
            createPasswordSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'ApiPort':
          {
            createIntSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'ApiUser':
          {
            createStringSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              Icons.person,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'DatabaseType':
          {
            createStringListSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              constants.databaseTypeList,
              Icons.dataset,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
            );
          }
          break;
        case 'DownloadTimeoutSeconds':
          {
            createIntSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'EnableDownloadTimeout':
          {
            createBooleanSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
            );
          }
          break;
        case 'EnableDriveEvents':
          {
            createBooleanSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
            );
          }
          break;
        case 'EventLevel':
          {
            createStringListSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              constants.eventLevelList,
              Icons.event,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
            );
          }
          break;
        case 'EvictionDelayMinutes':
          {
            createIntSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'EvictionUseAccessedTime':
          {
            createBooleanSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
            );
          }
          break;
        case 'MaxCacheSizeBytes':
          {
            createIntSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'MaxUploadCount':
          {
            createIntSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'OnlineCheckRetrySeconds':
          {
            createIntSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'PreferredDownloadType':
          {
            createStringListSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              constants.downloadTypeList,
              Icons.download,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
            );
          }
          break;
        case 'RetryReadCount':
          {
            createIntSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: getSettingValidators(key),
            );
          }
          break;
        case 'RingBufferFileSize':
          {
            createIntListSetting(
              context,
              commonSettings,
              widget.settings,
              key,
              value,
              constants.ringBufferSizeList,
              512,
              Icons.animation,
              false,
              widget.showAdvanced,
              widget,
              setState,
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
                createStringSetting(
                  context,
                  siaConfigSettings,
                  widget.settings[key],
                  subKey,
                  subValue,
                  Icons.folder,
                  false,
                  widget.showAdvanced,
                  widget,
                  setState,
                  description: getSettingDescription('$key.$subKey'),
                  validators: [
                    ...getSettingValidators('$key.$subKey'),
                    (value) => !Provider.of<MountList>(context, listen: false)
                        .hasBucketName(
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

    final titleStyle = theme.textTheme.titleLarge?.copyWith(
      fontWeight: FontWeight.w700,
      color: scheme.onSurface,
    );

    return SettingsList(
      shrinkWrap: false,
      platform: DevicePlatform.web,
      lightTheme: settingsTheme,
      darkTheme: settingsTheme,
      sections: [
        if (encryptConfigSettings.isNotEmpty)
          SettingsSection(
            title: Text('Encrypt Config', style: titleStyle),
            tiles: encryptConfigSettings,
          ),
        if (hostConfigSettings.isNotEmpty)
          SettingsSection(
            title: Text('Host Config', style: titleStyle),
            tiles: hostConfigSettings,
          ),
        if (remoteConfigSettings.isNotEmpty)
          SettingsSection(
            title: Text('Remote Config', style: titleStyle),
            tiles: remoteConfigSettings,
          ),
        if (s3ConfigSettings.isNotEmpty)
          SettingsSection(
            title: Text('S3 Config', style: titleStyle),
            tiles: s3ConfigSettings,
          ),
        if (siaConfigSettings.isNotEmpty)
          SettingsSection(
            title: Text('Sia Config', style: titleStyle),
            tiles: siaConfigSettings,
          ),
        if (remoteMountSettings.isNotEmpty)
          SettingsSection(
            title: Text('Remote Mount', style: titleStyle),
            tiles: (widget.settings['RemoteMount']['Enable'] as bool)
                ? remoteMountSettings
                : [remoteMountSettings[0]],
          ),
        if (commonSettings.isNotEmpty)
          SettingsSection(
            title: Text('Settings', style: titleStyle),
            tiles: commonSettings,
          ),
      ],
    );
  }

  void _parseHostConfig(List<SettingsTile> hostConfigSettings, key, value) {
    value.forEach((subKey, subValue) {
      switch (subKey) {
        case 'AgentString':
          {
            createStringSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.support_agent,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ApiPassword':
          {
            createPasswordSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ApiPort':
          {
            createIntSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ApiUser':
          {
            createStringSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.person,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'HostNameOrIp':
          {
            createStringSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.computer,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Path':
          {
            createStringSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.route,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Protocol':
          {
            createStringListSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              constants.protocolTypeList,
              Icons.http,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
            );
          }
          break;
        case 'TimeoutMs':
          {
            createIntSetting(
              context,
              hostConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              widget.showAdvanced,
              widget,
              setState,
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
            createPasswordSetting(
              context,
              encryptConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Path':
          {
            createStringSetting(
              context,
              encryptConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.folder,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
      }
    });
  }

  void _parseS3Config(List<SettingsTile> s3ConfigSettings, String key, value) {
    value.forEach((subKey, subValue) {
      switch (subKey) {
        case 'AccessKey':
          {
            createStringSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.key,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'Bucket':
          {
            createStringSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.folder,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: [
                ...getSettingValidators('$key.$subKey'),
                (value) => !Provider.of<MountList>(context, listen: false)
                    .hasBucketName(
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
            createPasswordSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ForceLegacyEncryption':
          {
            createBooleanSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
            );
          }
          break;
        case 'Region':
          {
            createStringSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.map,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'SecretKey':
          {
            createPasswordSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'TimeoutMs':
          {
            createIntSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'URL':
          {
            createStringSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.http,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'UsePathStyle':
          {
            createBooleanSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
            );
          }
          break;
        case 'UseRegionInURL':
          {
            createBooleanSetting(
              context,
              s3ConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
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
            createBooleanSetting(
              context,
              tempSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
            );
            remoteMountSettings.insertAll(0, tempSettings);
          }
          break;
        case 'ApiPort':
          {
            createIntSetting(
              context,
              remoteMountSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ClientPoolSize':
          {
            createIntSetting(
              context,
              remoteMountSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'EncryptionToken':
          {
            createPasswordSetting(
              context,
              remoteMountSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
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
            createIntSetting(
              context,
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ConnectTimeoutMs':
          {
            createIntSetting(
              context,
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'EncryptionToken':
          {
            createPasswordSetting(
              context,
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'HostNameOrIp':
          {
            createStringSetting(
              context,
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              Icons.computer,
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'MaxConnections':
          {
            createIntSetting(
              context,
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'ReceiveTimeoutMs':
          {
            createIntSetting(
              context,
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription('$key.$subKey'),
              validators: getSettingValidators('$key.$subKey'),
            );
          }
          break;
        case 'SendTimeoutMs':
          {
            createIntSetting(
              context,
              remoteConfigSettings,
              widget.settings[key],
              subKey,
              subValue,
              true,
              widget.showAdvanced,
              widget,
              setState,
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
      final settings = getChanged(
        widget.mount.mountConfig.settings,
        widget.settings,
      );
      if (settings.isNotEmpty) {
        final mount = widget.mount;
        final key = Provider.of<Auth>(
          constants.navigatorKey.currentContext!,
          listen: false,
        ).key;
        convertAllToString(settings, key).then((map) {
          map.forEach((key, value) {
            if (value is Map<String, dynamic>) {
              value.forEach((subKey, subValue) {
                mount.setValue('$key.$subKey', subValue);
              });
              return;
            }
            mount.setValue(key, value);
          });
        });
      }
    }
    super.dispose();
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    if (widget.onChanged != null) {
      widget.onChanged!();
    }

    super.setState(fn);
  }
}
