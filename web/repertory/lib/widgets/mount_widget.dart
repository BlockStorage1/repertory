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
            trailing: IconButton(
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
              onPressed: _createMountHandler(context, mount),
            ),
          );
        },
      ),
    );
  }

  VoidCallback? _createMountHandler(context, Mount mount) {
    return _enabled && mount.mounted != null
        ? () async {
          setState(() {
            _enabled = false;
          });

          final location = await _getMountLocation(context, mount);

          cleanup() {
            setState(() {
              _enabled = true;
            });
          }

          if (!mount.mounted! && location == null) {
            displayErrorMessage(context, "Mount location is not set");
            return cleanup();
          }

          final success = await mount.mount(mount.mounted!, location: location);

          if (success ||
              mount.mounted! ||
              constants.navigatorKey.currentContext == null ||
              !constants.navigatorKey.currentContext!.mounted) {
            return cleanup();
          }

          displayErrorMessage(context, "Mount location is not available");
          return cleanup();
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

    String? currentLocation;
    return await showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          actions: [
            TextButton(
              child: const Text('Cancel'),
              onPressed: () => Navigator.of(context).pop(null),
            ),
            TextButton(
              child: const Text('OK'),
              onPressed: () {
                final result = getSettingValidators('Path').firstWhereOrNull(
                  (validator) => !validator(currentLocation ?? ''),
                );
                if (result != null) {
                  return displayErrorMessage(
                    context,
                    "Mount location is not valid",
                  );
                }
                Navigator.of(context).pop(currentLocation);
              },
            ),
          ],
          content: TextField(
            autofocus: true,
            controller: TextEditingController(text: currentLocation),
            inputFormatters: [FilteringTextInputFormatter.deny(RegExp(r'\s'))],
            onChanged: (value) => currentLocation = value,
          ),
          title: const Text('Set Mount Location'),
        );
      },
    );
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
