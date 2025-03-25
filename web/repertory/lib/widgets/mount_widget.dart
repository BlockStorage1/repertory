import 'dart:async';

import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount.dart';

class MountWidget extends StatefulWidget {
  const MountWidget({super.key});

  @override
  State<MountWidget> createState() => _MountWidgetState();
}

class _MountWidgetState extends State<MountWidget> {
  bool _enabled = true;
  bool _editEnabled = true;
  Timer? _timer;

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.all(0.0),
      child: Consumer<Mount>(
        builder: (context, Mount mount, _) {
          final textColor = Theme.of(context).colorScheme.onSurface;
          final subTextColor =
              Theme.of(context).brightness == Brightness.dark
                  ? Colors.white38
                  : Colors.black87;

          final nameText = SelectableText(
            formatMountName(mount.type, mount.name),
            style: TextStyle(color: subTextColor),
          );

          return ListTile(
            isThreeLine: true,
            leading: IconButton(
              icon: Icon(Icons.settings, color: textColor),
              onPressed:
                  () => Navigator.pushNamed(context, '/edit', arguments: mount),
            ),
            subtitle: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                nameText,
                SelectableText(
                  mount.path.isEmpty && mount.mounted == null
                      ? 'loading...'
                      : mount.path.isEmpty
                      ? '<mount location not set>'
                      : mount.path,
                  style: TextStyle(color: subTextColor),
                ),
              ],
            ),
            title: SelectableText(
              mount.provider,
              style: TextStyle(color: textColor, fontWeight: FontWeight.bold),
            ),
            trailing: Row(
              mainAxisAlignment: MainAxisAlignment.end,
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                if (mount.mounted != null && !mount.mounted!)
                  IconButton(
                    icon: const Icon(Icons.edit),
                    color: subTextColor,
                    tooltip: 'Edit mount location',
                    onPressed: () async {
                      setState(() => _editEnabled = false);
                      final available = await mount.getAvailableLocations();
                      if (context.mounted) {
                        final location = await editMountLocation(
                          context,
                          available,
                          location: mount.path,
                        );
                        if (location != null) {
                          await mount.setMountLocation(location);
                        }
                      }
                      setState(() => _editEnabled = true);
                    },
                  ),
                IconButton(
                  icon: Icon(
                    mount.mounted == null
                        ? Icons.hourglass_top
                        : mount.mounted!
                        ? Icons.toggle_on
                        : Icons.toggle_off,
                    color:
                        mount.mounted ?? false
                            ? Color.fromARGB(255, 163, 96, 76)
                            : subTextColor,
                  ),
                  tooltip:
                      mount.mounted == null
                          ? ''
                          : mount.mounted!
                          ? 'Unmount'
                          : 'Mount',
                  onPressed: _createMountHandler(context, mount),
                ),
              ],
            ),
          );
        },
      ),
    );
  }

  VoidCallback? _createMountHandler(context, Mount mount) {
    return _enabled && mount.mounted != null
        ? () async {
          if (mount.mounted == null) {
            return;
          }

          final mounted = mount.mounted!;

          setState(() {
            _enabled = false;
          });

          final location = await _getMountLocation(context, mount);

          cleanup() {
            setState(() {
              _enabled = true;
            });
          }

          if (!mounted && location == null) {
            displayErrorMessage(context, "Mount location is not set");
            return cleanup();
          }

          final success = await mount.mount(mounted, location: location);
          if (success ||
              mounted ||
              constants.navigatorKey.currentContext == null ||
              !constants.navigatorKey.currentContext!.mounted) {
            return cleanup();
          }

          displayErrorMessage(
            context,
            "Mount location is not available: $location",
          );
          cleanup();
        }
        : null;
  }

  @override
  void dispose() {
    _timer?.cancel();
    _timer = null;
    super.dispose();
  }

  Future<String?> _getMountLocation(context, Mount mount) async {
    if (mount.mounted ?? false) {
      return null;
    }

    if (mount.path.isNotEmpty) {
      return mount.path;
    }

    String? location = await mount.getMountLocation();
    if (location != null) {
      return location;
    }

    if (!context.mounted) {
      return location;
    }

    return editMountLocation(context, await mount.getAvailableLocations());
  }

  @override
  void initState() {
    super.initState();
    _timer = Timer.periodic(const Duration(seconds: 5), (_) {
      Provider.of<Mount>(context, listen: false).refresh();
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
