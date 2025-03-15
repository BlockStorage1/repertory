import 'dart:async';

import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount.dart';

class MountWidget extends StatefulWidget {
  const MountWidget({super.key});

  @override
  State<MountWidget> createState() => _MountWidgetState();
}

class _MountWidgetState extends State<MountWidget> {
  Timer? _timer;
  bool _enabled = true;

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.all(0.0),
      child: Consumer<Mount>(
        builder: (context, mount, _) {
          final textColor = Theme.of(context).colorScheme.onSurface;
          final subTextColor =
              Theme.of(context).brightness == Brightness.dark
                  ? Colors.white38
                  : Colors.black87;

          final isActive = mount.state == Icons.toggle_on;
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
                  mount.path,
                  style: TextStyle(color: subTextColor),
                ),
              ],
            ),
            title: SelectableText(
              initialCaps(mount.type),
              style: TextStyle(color: textColor, fontWeight: FontWeight.bold),
            ),
            trailing: IconButton(
              icon: Icon(
                mount.state,
                color:
                    isActive ? Color.fromARGB(255, 163, 96, 76) : subTextColor,
              ),
              onPressed:
                  _enabled
                      ? () async {
                        setState(() {
                          _enabled = false;
                        });

                        String? location = mount.path;
                        if (!isActive && mount.path.isEmpty) {
                          location = await mount.getMountLocation();
                          if (location == null) {
                            String? updatedValue;
                            if (context.mounted) {
                              location = await showDialog(
                                context: context,
                                builder: (context) {
                                  return AlertDialog(
                                    actions: [
                                      TextButton(
                                        child: const Text('Cancel'),
                                        onPressed:
                                            () =>
                                                Navigator.of(context).pop(null),
                                      ),
                                      TextButton(
                                        child: const Text('OK'),
                                        onPressed: () {
                                          final result = getSettingValidators(
                                            'Path',
                                          ).firstWhereOrNull(
                                            (validator) =>
                                                !validator(updatedValue ?? ''),
                                          );
                                          if (result != null) {
                                            ScaffoldMessenger.of(
                                              context,
                                            ).showSnackBar(
                                              SnackBar(
                                                content: const Text(
                                                  "mount location is not valid",
                                                  textAlign: TextAlign.center,
                                                ),
                                              ),
                                            );
                                            return;
                                          }
                                          Navigator.of(
                                            context,
                                          ).pop(updatedValue);
                                        },
                                      ),
                                    ],
                                    content: TextField(
                                      autofocus: true,
                                      controller: TextEditingController(
                                        text: updatedValue,
                                      ),
                                      inputFormatters: [
                                        FilteringTextInputFormatter.deny(
                                          RegExp(r'\s'),
                                        ),
                                      ],
                                      onChanged:
                                          (value) => updatedValue = value,
                                    ),
                                    title: const Text('Set Mount Location'),
                                  );
                                },
                              );
                            }
                          }
                        }

                        if (location == null) {
                          if (!context.mounted) {
                            return;
                          }

                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(
                              content: const Text(
                                "mount location is not set",
                                textAlign: TextAlign.center,
                              ),
                            ),
                          );
                          return;
                        }

                        mount
                            .mount(isActive, location: location)
                            .then((_) {
                              setState(() {
                                _enabled = true;
                              });
                            })
                            .catchError((_) {
                              setState(() {
                                _enabled = true;
                              });
                            });
                      }
                      : null,
            ),
          );
        },
      ),
    );
  }

  @override
  void dispose() {
    _timer?.cancel();
    _timer = null;
    super.dispose();
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
