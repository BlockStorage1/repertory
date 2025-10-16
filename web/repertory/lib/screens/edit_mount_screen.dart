// edit_mount_screen.dart

import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/models/mount.dart';
import 'package:repertory/utils/safe_set_state_mixin.dart';
import 'package:repertory/widgets/app_scaffold.dart';
import 'package:repertory/widgets/mount_settings.dart';

class EditMountScreen extends StatefulWidget {
  final Mount mount;
  final String title;
  const EditMountScreen({super.key, required this.mount, required this.title});

  @override
  State<EditMountScreen> createState() => _EditMountScreenState();
}

class _EditMountScreenState extends State<EditMountScreen>
    with SafeSetState<EditMountScreen> {
  bool _showAdvanced = false;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final textTheme = Theme.of(context).textTheme;
    return AppScaffold(
      title: widget.title,
      showBack: true,
      advancedWidget: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            "Advanced",
            style: textTheme.labelLarge?.copyWith(color: scheme.onSurface),
          ),
          const SizedBox(width: 6),
          IconButton(
            tooltip: _showAdvanced ? 'Hide advanced' : 'Show advanced',
            icon: Icon(_showAdvanced ? Icons.toggle_on : Icons.toggle_off),
            color: _showAdvanced ? scheme.primary : scheme.onSurface,
            onPressed: () => setState(() => _showAdvanced = !_showAdvanced),
          ),
        ],
      ),
      children: [
        Expanded(
          child: MountSettingsWidget(
            mount: widget.mount,
            settings: jsonDecode(jsonEncode(widget.mount.mountConfig.settings)),
            showAdvanced: _showAdvanced,
          ),
        ),
        const SizedBox(height: constants.padding),
      ],
    );
  }
}
