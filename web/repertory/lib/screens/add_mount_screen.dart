import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart';
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
  static const _padding = 15.0;

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
        padding: const EdgeInsets.all(_padding),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.start,
          mainAxisSize: MainAxisSize.max,
          children: [
            Card(
              child: Padding(
                padding: const EdgeInsets.all(_padding),
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.start,
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    const Text('Provider Type'),
                    const SizedBox(width: _padding),
                    DropdownButton<String>(
                      value: _mountType,
                      onChanged: (mountType) => _handleChange(mountType ?? ''),
                      items:
                          providerTypeList.map<DropdownMenuItem<String>>((
                            item,
                          ) {
                            return DropdownMenuItem<String>(
                              value: item,
                              child: Text(item),
                            );
                          }).toList(),
                    ),
                  ],
                ),
              ),
            ),
            if (_mountType.isNotEmpty) const SizedBox(height: _padding),
            if (_mountType.isNotEmpty)
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(_padding),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisAlignment: MainAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      const Text('Configuration Name'),
                      const SizedBox(width: _padding),
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
            if (_mount != null)
              Expanded(
                child: Card(
                  child: Padding(
                    padding: const EdgeInsets.all(_padding),
                    child: MountSettingsWidget(
                      isAdd: true,
                      mount: _mount!,
                      settings: _settings[_mountType]!,
                      showAdvanced: _showAdvanced,
                    ),
                  ),
                ),
              ),
            if (_mount != null)
              ElevatedButton.icon(
                onPressed: () {
                  List<String> failed = [];
                  if (!validateSettings(_settings[_mountType]!, failed)) {
                    return;
                  }

                  Provider.of<MountList>(context, listen: false).add(
                    _mountType,
                    _mountNameController.text,
                    _settings[_mountType]!,
                  );

                  Navigator.pop(context);
                },
                label: const Text('Add'),
                icon: Icon(Icons.add),
              ),
          ],
        ),
      ),
    );
  }

  void _handleChange(String mountType) {
    setState(() {
      _mountType = mountType;
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
