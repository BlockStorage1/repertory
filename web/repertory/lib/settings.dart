// settings.dart

import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart'
    show Validator, displayErrorMessage, doShowDialog;
import 'package:repertory/widgets/app_dropdown.dart';
import 'package:flutter_settings_ui/flutter_settings_ui.dart';

void createBooleanSetting(
  BuildContext context,
  List<Widget> list,
  Map<String, dynamic> settings,
  String key,
  value,
  bool isAdvanced,
  bool showAdvanced,
  widget,
  Function setState, {
  String? description,
  IconData icon = Icons.quiz,
}) {
  if (!isAdvanced || showAdvanced) {
    list.add(
      SettingsTile.switchTile(
        leading: Icon(icon),
        title: createSettingTitle(context, key, description),
        initialValue: (value as bool),
        onPressed: (_) => setState(() => settings[key] = !value),
        onToggle: (bool nextValue) {
          setState(() => settings[key] = nextValue);
        },
      ),
    );
  }
}

void createIntListSetting(
  BuildContext context,
  List<Widget> list,
  Map<String, dynamic> settings,
  String key,
  value,
  List<String> valueList,
  defaultValue,
  IconData icon,
  bool isAdvanced,
  bool showAdvanced,
  widget,
  Function setState, {
  String? description,
}) {
  if (!isAdvanced || widget.showAdvanced) {
    list.add(
      SettingsTile.navigation(
        title: createSettingTitle(context, key, description),
        leading: Icon(icon),
        value: AppDropdown<String>(
          labelOf: (s) => s,
          constrainToIntrinsic: true,
          onChanged: (newValue) {
            setState(
              () => settings[key] = int.parse(
                newValue ?? defaultValue.toString(),
              ),
            );
          },
          value: value.toString(),
          values: valueList,
        ),
      ),
    );
  }
}

void createIntSetting(
  BuildContext context,
  List<Widget> list,
  Map<String, dynamic> settings,
  String key,
  value,
  bool isAdvanced,
  bool showAdvanced,
  widget,
  Function setState, {
  String? description,
  IconData icon = Icons.onetwothree,
  List<Validator> validators = const [],
}) {
  if (!isAdvanced || widget.showAdvanced) {
    list.add(
      SettingsTile.navigation(
        leading: Icon(icon),
        title: createSettingTitle(context, key, description),
        value: Text(value.toString()),
        onPressed: (_) {
          String updatedValue = value.toString();
          doShowDialog(
            context,
            AlertDialog(
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
                    setState(() => settings[key] = int.parse(updatedValue));
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
              title: createSettingTitle(context, key, description),
            ),
          );
        },
      ),
    );
  }
}

void createPasswordSetting(
  BuildContext context,
  List<Widget> list,
  Map<String, dynamic> settings,
  String key,
  value,
  bool isAdvanced,
  bool showAdvanced,
  widget,
  Function setState, {
  String? description,
  IconData icon = Icons.password,
  List<Validator> validators = const [],
}) {
  if (!isAdvanced || widget.showAdvanced) {
    list.add(
      SettingsTile.navigation(
        leading: Icon(icon),
        title: createSettingTitle(context, key, description),
        value: Text('*' * (value as String).length),
        onPressed: (_) {
          String updatedValue1 = value;
          String updatedValue2 = value;
          bool hidePassword1 = true;
          bool hidePassword2 = true;
          doShowDialog(
            context,
            StatefulBuilder(
              builder: (context, setDialogState) {
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

                        setState(() => settings[key] = updatedValue1);
                        Navigator.of(context).pop();
                      },
                    ),
                  ],
                  content: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisAlignment: MainAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Row(
                        children: [
                          Expanded(
                            child: TextField(
                              autofocus: true,
                              controller: TextEditingController(
                                text: updatedValue1,
                              ),
                              obscureText: hidePassword1,
                              obscuringCharacter: '*',
                              onChanged: (value) => updatedValue1 = value,
                            ),
                          ),
                          IconButton(
                            onPressed: () => setDialogState(
                              () => hidePassword1 = !hidePassword1,
                            ),
                            icon: Icon(
                              hidePassword1
                                  ? Icons.visibility
                                  : Icons.visibility_off,
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: constants.padding),
                      Row(
                        children: [
                          Expanded(
                            child: TextField(
                              autofocus: false,
                              controller: TextEditingController(
                                text: updatedValue2,
                              ),
                              obscureText: hidePassword2,
                              obscuringCharacter: '*',
                              onChanged: (value) => updatedValue2 = value,
                            ),
                          ),
                          IconButton(
                            onPressed: () => setDialogState(
                              () => hidePassword2 = !hidePassword2,
                            ),
                            icon: Icon(
                              hidePassword2
                                  ? Icons.visibility
                                  : Icons.visibility_off,
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                  title: createSettingTitle(context, key, description),
                );
              },
            ),
          );
        },
      ),
    );
  }
}

Widget createSettingTitle(
  BuildContext context,
  String key,
  String? description,
) {
  if (description == null) {
    return Text(key);
  }

  return Column(
    mainAxisAlignment: MainAxisAlignment.start,
    mainAxisSize: MainAxisSize.min,
    crossAxisAlignment: CrossAxisAlignment.start,
    children: [
      Text(key, textAlign: TextAlign.start),
      Text(
        description,
        style: Theme.of(context).textTheme.titleSmall,
        textAlign: TextAlign.start,
      ),
    ],
  );
}

void createStringListSetting(
  BuildContext context,
  List<Widget> list,
  Map<String, dynamic> settings,
  String key,
  value,
  List<String> valueList,
  IconData icon,
  bool isAdvanced,
  bool showAdvanced,
  widget,
  Function setState, {
  String? description,
}) {
  if (!isAdvanced || widget.showAdvanced) {
    list.add(
      SettingsTile.navigation(
        title: createSettingTitle(context, key, description),
        leading: Icon(icon),
        value: AppDropdown<String>(
          constrainToIntrinsic: true,
          labelOf: (s) => s,
          onChanged: (newValue) => setState(() => settings[key] = newValue),
          value: value,
          values: valueList,
        ),
      ),
    );
  }
}

void createStringSetting(
  BuildContext context,
  List<Widget> list,
  Map<String, dynamic> settings,
  String key,
  value,
  IconData icon,
  bool isAdvanced,
  bool showAdvanced,
  widget,
  Function setState, {
  String? description,
  List<Validator> validators = const [],
}) {
  if (!isAdvanced || widget.showAdvanced) {
    list.add(
      SettingsTile.navigation(
        leading: Icon(icon),
        title: createSettingTitle(context, key, description),
        value: Text(value),
        onPressed: (_) {
          String updatedValue = value;
          doShowDialog(
            context,
            AlertDialog(
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
                    setState(() => settings[key] = updatedValue);
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
              title: createSettingTitle(context, key, description),
            ),
          );
        },
      ),
    );
  }
}
