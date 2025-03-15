import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
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
  bool _showAdvanced = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
        actions: [
          Row(
            children: [
              const Text("Advanced"),
              IconButton(
                icon: Icon(_showAdvanced ? Icons.toggle_on : Icons.toggle_off),
                onPressed: () => setState(() => _showAdvanced = !_showAdvanced),
              ),
            ],
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(constants.padding),
        child: Column(
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
                      value: _mountType,
                      onChanged: (mountType) {
                        _handleChange(mountType ?? '');
                      },
                      items:
                          constants.providerTypeList
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
            if (_mountType.isNotEmpty)
              const SizedBox(height: constants.padding),
            if (_mountType.isNotEmpty)
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
                        onChanged: (_) => _handleChange(_mountType),
                      ),
                    ],
                  ),
                ),
              ),
            if (_mount != null) const SizedBox(height: constants.padding),
            if (_mount != null)
              Expanded(
                child: Card(
                  margin: EdgeInsets.all(0.0),
                  child: Padding(
                    padding: const EdgeInsets.all(constants.padding),
                    child: MountSettingsWidget(
                      isAdd: true,
                      mount: _mount!,
                      settings: _settings[_mountType]!,
                      showAdvanced: _showAdvanced,
                    ),
                  ),
                ),
              ),
            if (_mount != null) const SizedBox(height: constants.padding),
            if (_mount != null)
              Builder(
                builder: (context) {
                  return ElevatedButton.icon(
                    onPressed: () {
                      final mountList = Provider.of<MountList>(
                        context,
                        listen: false,
                      );

                      List<String> failed = [];
                      if (!validateSettings(_settings[_mountType]!, failed)) {
                        for (var key in failed) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(
                              content: Text(
                                "Setting '$key' is not valid",
                                textAlign: TextAlign.center,
                              ),
                            ),
                          );
                        }
                        return;
                      }

                      if (mountList.hasConfigName(_mountNameController.text)) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          SnackBar(
                            content: Text(
                              "Configuration '${_mountNameController.text}' already exists",
                              textAlign: TextAlign.center,
                            ),
                          ),
                        );
                        return;
                      }

                      if (_mountType == "Sia" || _mountType == "S3") {
                        final bucket =
                            _settings[_mountType]!["${_mountType}Config"]["Bucket"]
                                as String;
                        if (mountList.hasBucketName(_mountType, bucket)) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(
                              content: Text(
                                "Bucket '$bucket' already exists",
                                textAlign: TextAlign.center,
                              ),
                            ),
                          );
                          return;
                        }
                      }

                      mountList.add(
                        _mountType,
                        _mountNameController.text,
                        _settings[_mountType]!,
                      );

                      Navigator.pop(context);
                    },
                    label: const Text('Add'),
                    icon: const Icon(Icons.add),
                  );
                },
              ),
          ],
        ),
      ),
    );
  }

  void _handleChange(String mountType) {
    setState(() {
      final changed = _mountType != mountType;

      _mountType = mountType;
      _mountNameController.text =
          (mountType == 'Sia' && changed)
              ? 'default'
              : changed
              ? ''
              : _mountNameController.text;

      _mount =
          (_mountNameController.text.isEmpty)
              ? null
              : Mount(
                MountConfig(
                  name: _mountNameController.text,
                  settings: _settings[mountType],
                  type: mountType,
                ),
                isAdd: true,
              );
    });
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
