import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/widgets/mount_settings.dart';

class EditMountScreen extends StatefulWidget {
  final Mount mount;
  final String title;
  const EditMountScreen({super.key, required this.mount, required this.title});

  @override
  State<EditMountScreen> createState() => _EditMountScreenState();
}

class _EditMountScreenState extends State<EditMountScreen> {
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
              Row(
                children: [
                  const Text("Advanced"),
                  IconButton(
                    icon: Icon(
                      _showAdvanced ? Icons.toggle_on : Icons.toggle_off,
                    ),
                    onPressed:
                        () => setState(() => _showAdvanced = !_showAdvanced),
                  ),
                ],
              ),
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
        ],
      ),
      body: MountSettingsWidget(
        mount: widget.mount,
        settings: jsonDecode(jsonEncode(widget.mount.mountConfig.settings)),
        showAdvanced: _showAdvanced,
      ),
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
