import 'dart:convert' show jsonEncode;

import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/helpers.dart'
    show
        convertAllToString,
        getBaseUri,
        getSettingDescription,
        getSettingValidators,
        trimNotEmptyValidator;
import 'package:repertory/settings.dart';
import 'package:settings_ui/settings_ui.dart';

class UISettingsWidget extends StatefulWidget {
  final bool showAdvanced;
  final Map<String, dynamic> settings;
  final Map<String, dynamic> origSettings;
  const UISettingsWidget({
    super.key,
    required this.origSettings,
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
  void dispose() {
    if (!DeepCollectionEquality().equals(
      widget.settings,
      widget.origSettings,
    )) {
      convertAllToString(widget.settings)
          .then((map) async {
            try {
              await http.put(
                Uri.parse(
                  Uri.encodeFull(
                    '${getBaseUri()}/api/v1/settings?data=${jsonEncode(map)}',
                  ),
                ),
              );
            } catch (e) {
              debugPrint('$e');
            }
          })
          .catchError((e) {
            debugPrint('$e');
          });
    }

    super.dispose();
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
