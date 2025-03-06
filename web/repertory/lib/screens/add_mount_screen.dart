import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:repertory/constants.dart';
import 'package:repertory/models/mount.dart';
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

  late TextEditingController _mountNameController;

  Mount? _mount;
  String _mountName = "";
  String _mountType = "";
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
          mainAxisSize: MainAxisSize.min,
          children: [
            Card(
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  const Text('Provider Type'),
                  const SizedBox(width: _padding),
                  DropdownButton<String>(
                    value: _mountType,
                    onChanged: (newValue) {
                      setState(() {
                        _mountType = newValue ?? "";
                        if (_mountType.isNotEmpty) {}
                      });
                    },
                    items:
                        providerTypeList.map<DropdownMenuItem<String>>((item) {
                          return DropdownMenuItem<String>(
                            value: item,
                            child: Text(item),
                          );
                        }).toList(),
                  ),
                ],
              ),
            ),
            if (_mountType.isNotEmpty) const SizedBox(height: _padding),
            if (_mountType.isNotEmpty)
              Card(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    const Text('Configuration Name'),
                    const SizedBox(width: _padding),
                    TextField(
                      autofocus: true,
                      controller: _mountNameController,
                      keyboardType: TextInputType.number,
                      onChanged: (value) {
                        if (_mountName == value) {
                          return;
                        }

                        setState(() {
                          _mountName = value;
                          _mount =
                              (_mountName.isEmpty)
                                  ? null
                                  : Mount(
                                    MountConfig(
                                      name: _mountName,
                                      type: _mountType,
                                    ),
                                  );
                        });
                      },
                    ),
                  ],
                ),
              ),
            if (_mount != null)
              MountSettingsWidget(mount: _mount!, showAdvanced: _showAdvanced),
          ],
        ),
      ),
    );
  }

  @override
  void initState() {
    _mountNameController = TextEditingController(text: _mountName);
    super.initState();
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
