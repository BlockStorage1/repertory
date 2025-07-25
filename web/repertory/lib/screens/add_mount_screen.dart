import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/types/mount_config.dart';
import 'package:repertory/widgets/mount_settings.dart';

class AddMountScreen extends StatefulWidget {
  final String title;
  const AddMountScreen({super.key, required this.title});

  @override
  State<AddMountScreen> createState() => _AddMountScreenState();
}

class _AddMountScreenState extends State<AddMountScreen> {
  Mount? _mount;
  final _mountNameController = TextEditingController();
  String _mountType = "";

  final Map<String, Map<String, dynamic>> _settings = {
    "": {},
    "Encrypt": createDefaultSettings("Encrypt"),
    "Remote": createDefaultSettings("Remote"),
    "S3": createDefaultSettings("S3"),
    "Sia": createDefaultSettings("Sia"),
  };

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
        actions: [
          Consumer<Auth>(
            builder: (context, auth, _) {
              return IconButton(
                icon: const Icon(Icons.logout),
                onPressed: () => auth.logoff(),
              );
            },
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(constants.padding),
        child: Consumer<Auth>(
          builder: (context, auth, _) {
            return Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.start,
              mainAxisSize: MainAxisSize.max,
              children: [
                Card(
                  margin: EdgeInsets.all(0.0),
                  child: Padding(
                    padding: const EdgeInsets.all(constants.padding),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisAlignment: MainAxisAlignment.start,
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        const Text('Provider Type'),
                        const SizedBox(width: constants.padding),
                        DropdownButton<String>(
                          autofocus: true,
                          value: _mountType,
                          onChanged: (mountType) =>
                              _handleChange(auth, mountType ?? ''),
                          items: constants.providerTypeList
                              .map<DropdownMenuItem<String>>((item) {
                                return DropdownMenuItem<String>(
                                  value: item,
                                  child: Text(item),
                                );
                              })
                              .toList(),
                        ),
                      ],
                    ),
                  ),
                ),
                if (_mountType.isNotEmpty && _mountType != 'Remote')
                  const SizedBox(height: constants.padding),
                if (_mountType.isNotEmpty && _mountType != 'Remote')
                  Card(
                    margin: EdgeInsets.all(0.0),
                    child: Padding(
                      padding: const EdgeInsets.all(constants.padding),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        mainAxisAlignment: MainAxisAlignment.start,
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Text('Configuration Name'),
                          const SizedBox(width: constants.padding),
                          TextField(
                            autofocus: true,
                            controller: _mountNameController,
                            keyboardType: TextInputType.text,
                            inputFormatters: [
                              FilteringTextInputFormatter.deny(RegExp(r'\s')),
                            ],
                            onChanged: (_) => _handleChange(auth, _mountType),
                          ),
                        ],
                      ),
                    ),
                  ),
                if (_mount != null) ...[
                  const SizedBox(height: constants.padding),
                  Expanded(
                    child: Card(
                      margin: EdgeInsets.all(0.0),
                      child: Padding(
                        padding: const EdgeInsets.all(constants.padding),
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
                      ElevatedButton.icon(
                        label: const Text('Test'),
                        icon: const Icon(Icons.check),
                        onPressed: _handleProviderTest,
                      ),
                      const SizedBox(width: constants.padding),
                      ElevatedButton.icon(
                        label: const Text('Add'),
                        icon: const Icon(Icons.add),
                        onPressed: () async {
                          final mountList = Provider.of<MountList>(context);

                          List<String> failed = [];
                          if (!validateSettings(
                            _settings[_mountType]!,
                            failed,
                          )) {
                            for (var key in failed) {
                              displayErrorMessage(
                                context,
                                "Setting '$key' is not valid",
                              );
                            }
                            return;
                          }

                          if (mountList.hasConfigName(
                            _mountNameController.text,
                          )) {
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
                        },
                      ),
                    ],
                  ),
                ],
              ],
            );
          },
        ),
      ),
    );
  }

  void _handleChange(Auth auth, String mountType) {
    setState(() {
      final changed = _mountType != mountType;

      _mountType = mountType;
      if (_mountType == 'Remote') {
        _mountNameController.text = 'remote';
      } else if (changed) {
        _mountNameController.text = mountType == 'Sia' ? 'default' : '';
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
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
