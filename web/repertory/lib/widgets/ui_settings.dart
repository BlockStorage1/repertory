// ui_settings.dart

import 'dart:convert' show jsonEncode;

import 'package:flutter/material.dart';
import 'package:flutter_settings_ui/flutter_settings_ui.dart'
    show DevicePlatform, SettingsList, SettingsTile;
import 'package:http/http.dart' as http;
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart'
    show
        convertAllToString,
        createSettingsTheme,
        displayAuthError,
        getBaseUri,
        getChanged,
        getSettingDescription,
        getSettingValidators,
        trimNotEmptyValidator;
import 'package:repertory/models/auth.dart';
import 'package:repertory/settings.dart';
import 'package:repertory/utils/safe_set_state_mixin.dart';
import 'package:repertory/widgets/settings/settings_section.dart';

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

class _UISettingsWidgetState extends State<UISettingsWidget>
    with SafeSetState<UISettingsWidget> {
  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final scheme = theme.colorScheme;
    final settingsTheme = createSettingsTheme(scheme);

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
        SettingsSection(
          title: Text('Settings', style: titleStyle),
          tiles: commonSettings,
        ),
      ],
    );
  }

  @override
  void dispose() {
    final settings = getChanged(widget.origSettings, widget.settings);
    if (settings.isNotEmpty) {
      final key = Provider.of<Auth>(
        constants.navigatorKey.currentContext!,
        listen: false,
      ).key;
      convertAllToString(settings, key)
          .then((map) async {
            try {
              final authProvider = Provider.of<Auth>(
                constants.navigatorKey.currentContext!,
                listen: false,
              );

              final auth = await authProvider.createAuth();
              final response = await http.put(
                Uri.parse(
                  Uri.encodeFull(
                    '${getBaseUri()}/api/v1/settings?auth=$auth&data=${jsonEncode(map)}',
                  ),
                ),
              );

              if (response.statusCode == 401) {
                displayAuthError(authProvider);
                authProvider.logoff();
              }
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
}
