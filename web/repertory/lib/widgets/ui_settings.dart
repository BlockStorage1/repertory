import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart'
    show getSettingDescription, getSettingValidators, trimNotEmptyValidator;
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/settings.dart';
import 'package:settings_ui/settings_ui.dart';

class UISettingsWidget extends StatefulWidget {
  final bool showAdvanced;
  final Function? onChanged;
  final Map<String, dynamic> settings;
  const UISettingsWidget({
    super.key,
    this.onChanged,
    required this.settings,
    required this.showAdvanced,
  });

  @override
  State<UISettingsWidget> createState() => _UISettingsWidgetState();
}

class _UISettingsWidgetState extends State<UISettingsWidget> {
  @override
  Widget build(BuildContext context) {
    List<SettingsTile> commonSettings = [];

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
              false,
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
              false,
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
              false,
              widget.showAdvanced,
              widget,
              setState,
              description: getSettingDescription(key),
              validators: [...getSettingValidators(key), trimNotEmptyValidator],
            );
          }
          break;
      }
    });

    return SettingsList(
      shrinkWrap: false,
      sections: [
        SettingsSection(title: const Text('Settings'), tiles: commonSettings),
      ],
    );
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
