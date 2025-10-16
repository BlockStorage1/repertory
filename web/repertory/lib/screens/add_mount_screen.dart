// add_mount_screen.dart

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/types/mount_config.dart';
import 'package:repertory/utils/safe_set_state_mixin.dart';
import 'package:repertory/widgets/app_dropdown.dart';
import 'package:repertory/widgets/app_outlined_icon_button.dart';
import 'package:repertory/widgets/app_scaffold.dart';
import 'package:repertory/widgets/mount_settings.dart';

class AddMountScreen extends StatefulWidget {
  final String title;
  const AddMountScreen({super.key, required this.title});

  @override
  State<AddMountScreen> createState() => _AddMountScreenState();
}

class _AddMountScreenState extends State<AddMountScreen>
    with SafeSetState<AddMountScreen> {
  Mount? _mount;
  final _mountNameController = TextEditingController();
  String _mountType = "";
  bool _enabled = true;

  final Map<String, Map<String, dynamic>> _settings = {
    "": {},
    "Encrypt": createDefaultSettings("Encrypt"),
    "Remote": createDefaultSettings("Remote"),
    "S3": createDefaultSettings("S3"),
    "Sia": createDefaultSettings("Sia"),
  };

  @override
  void dispose() {
    _mountNameController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    return AppScaffold(
      title: widget.title,
      showBack: true,
      children: [
        AppDropdown<String>(
          constrainToIntrinsic: true,
          isExpanded: false,
          labelOf: (s) => s,
          labelText: 'Provider Type',
          onChanged: (mountType) {
            _handleChange(
              Provider.of<Auth>(context, listen: false),
              mountType ?? '',
            );
          },
          prefixIcon: Icons.miscellaneous_services,
          value: _mountType.isEmpty ? null : _mountType,
          values: constants.providerTypeList,
          widthMultiplier: 2.0,
        ),
        if (_mountType.isNotEmpty && _mountType != 'Remote') ...[
          const SizedBox(height: constants.padding),
          TextField(
            autofocus: true,
            controller: _mountNameController,
            keyboardType: TextInputType.text,
            inputFormatters: [FilteringTextInputFormatter.deny(RegExp(r'\s'))],
            onChanged: (_) => _handleChange(
              Provider.of<Auth>(context, listen: false),
              _mountType,
            ),
            decoration: createCommonDecoration(
              scheme,
              'Configuration Name',
              hintText: 'Enter a unique name',
              icon: Icons.drive_file_rename_outline,
            ),
          ),
        ],
        if (_mount != null) ...[
          const SizedBox(height: constants.padding),
          Expanded(
            child: Padding(
              padding: const EdgeInsets.symmetric(
                horizontal: constants.padding,
              ),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(constants.borderRadius),
                child: MountSettingsWidget(
                  isAdd: true,
                  mount: _mount!,
                  settings: _settings[_mountType]!,
                  showAdvanced: false,
                ),
              ),
            ),
          ),
          const SizedBox(height: constants.padding),
          Row(
            children: [
              IntrinsicWidth(
                child: AppOutlinedIconButton(
                  enabled: _enabled,
                  icon: Icons.check,
                  text: 'Test',
                  onPressed: _handleProviderTest,
                ),
              ),
              const SizedBox(width: constants.padding),
              IntrinsicWidth(
                child: AppOutlinedIconButton(
                  enabled: _enabled,
                  icon: Icons.add,
                  text: 'Add',
                  onPressed: () async {
                    setState(() {
                      _enabled = false;
                    });

                    try {
                      final mountList = Provider.of<MountList>(
                        context,
                        listen: false,
                      );

                      List<String> failed = [];
                      if (!validateSettings(_settings[_mountType]!, failed)) {
                        for (var key in failed) {
                          displayErrorMessage(
                            context,
                            "Setting '$key' is not valid",
                          );
                        }
                        return;
                      }

                      if (mountList.hasConfigName(_mountNameController.text)) {
                        return displayErrorMessage(
                          context,
                          "Configuration name '${_mountNameController.text}' already exists",
                        );
                      }

                      if (_mountType == "Sia" || _mountType == "S3") {
                        final bucket =
                            _settings[_mountType]!["${_mountType}Config"]["Bucket"]
                                as String;
                        if (mountList.hasBucketName(_mountType, bucket)) {
                          return displayErrorMessage(
                            context,
                            "Bucket '$bucket' already exists",
                          );
                        }
                      }

                      final success = await mountList.add(
                        _mountType,
                        _mountType == 'Remote'
                            ? '${_settings[_mountType]!['RemoteConfig']['HostNameOrIp']}_${_settings[_mountType]!['RemoteConfig']['ApiPort']}'
                            : _mountNameController.text,
                        _settings[_mountType]!,
                      );

                      if (!success || !context.mounted) {
                        return;
                      }

                      Navigator.pop(context);
                    } finally {
                      setState(() {
                        _enabled = true;
                      });
                    }
                  },
                ),
              ),
            ],
          ),
        ],
      ],
    );
  }

  void _handleChange(Auth auth, String mountType) {
    setState(() {
      final changed = _mountType != mountType;

      _mountType = mountType;
      if (_mountType == 'Remote') {
        _mountNameController.text = 'remote';
      } else if (changed) {
        _mountNameController.text = '';
      }

      _mount = (_mountNameController.text.isEmpty)
          ? null
          : Mount(
              auth,
              MountConfig(
                name: _mountNameController.text,
                settings: _settings[mountType],
                type: mountType,
              ),
              null,
              isAdd: true,
            );
    });
  }

  Future<void> _handleProviderTest() async {
    setState(() {
      _enabled = false;
    });

    try {
      if (_mount == null) {
        return;
      }

      final success = await _mount!.test();
      if (!mounted) {
        return;
      }

      displayErrorMessage(
        context,
        success ? "Success" : "Provider settings are invalid!",
      );
    } finally {
      setState(() {
        _enabled = true;
      });
    }
  }
}
