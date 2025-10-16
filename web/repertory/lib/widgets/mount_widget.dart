// mount_widget.dart

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/utils/safe_set_state_mixin.dart';
import 'package:repertory/widgets/app_outlined_icon_button.dart';
import 'package:repertory/widgets/app_icon_button_framed.dart';
import 'package:repertory/widgets/app_toggle_button_framed.dart';

class MountWidget extends StatefulWidget {
  const MountWidget({super.key});

  @override
  State<MountWidget> createState() => _MountWidgetState();
}

class _MountWidgetState extends State<MountWidget>
    with SafeSetState<MountWidget> {
  bool _enabled = true;
  Timer? _timer;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final textTheme = Theme.of(context).textTheme;

    final titleStyle = textTheme.titleMedium?.copyWith(
      fontWeight: FontWeight.w700,
      color: scheme.onSurface,
    );
    final subStyle = textTheme.bodyMedium?.copyWith(color: scheme.onSurface);

    final visualDensity = VisualDensity(
      horizontal: -VisualDensity.maximumDensity,
      vertical: -(VisualDensity.maximumDensity * 2.0),
    );

    return ConstrainedBox(
      constraints: const BoxConstraints(minHeight: 120),
      child: Card(
        margin: const EdgeInsets.all(0.0),
        elevation: 12,
        color: scheme.primary.withValues(alpha: constants.primaryAlpha),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(constants.borderRadiusSmall),
          side: BorderSide(
            color: scheme.outlineVariant.withValues(
              alpha: constants.outlineAlpha,
            ),
            width: 1,
          ),
        ),
        child: Padding(
          padding: const EdgeInsets.all(constants.padding),
          child: Consumer<Mount>(
            builder: (context, Mount mount, _) {
              return Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    crossAxisAlignment: CrossAxisAlignment.center,
                    children: [
                      AppIconButtonFramed(
                        icon: Icons.settings,
                        onPressed: () => Navigator.pushNamed(
                          context,
                          '/edit',
                          arguments: mount,
                        ),
                      ),
                      const SizedBox(width: constants.padding),
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            SelectableText(mount.provider, style: titleStyle),
                            SelectableText(
                              'Name • ${formatMountName(mount.type, mount.name)}',
                              style: subStyle,
                            ),
                          ],
                        ),
                      ),
                      if (mount.mounted == false) ...[
                        AppIconButtonFramed(
                          icon: Icons.delete,
                          onPressed: _enabled
                              ? () async {
                                  setState(() {
                                    _enabled = false;
                                  });
                                  return doShowDialog(
                                    context,
                                    AlertDialog(
                                      actions: [
                                        TextButton(
                                          child: const Text('Yes'),
                                          onPressed: () async {
                                            await mount.remove();
                                            setState(() {
                                              _enabled = true;
                                            });
                                            Navigator.of(context).pop();
                                          },
                                        ),
                                        TextButton(
                                          child: const Text('No'),
                                          onPressed: () {
                                            setState(() {
                                              _enabled = true;
                                            });
                                            Navigator.of(context).pop();
                                          },
                                        ),
                                      ],
                                      title: const Text(
                                        'Are you sure?',
                                        textAlign: TextAlign.center,
                                      ),
                                    ),
                                  );
                                }
                              : null,
                        ),
                        SizedBox(width: constants.paddingSmall),
                      ],
                      AppToggleButtonFramed(
                        mounted: mount.mounted,
                        onPressed: _createMountHandler(context, mount),
                      ),
                    ],
                  ),
                  const SizedBox(height: constants.padding),
                  Row(
                    children: [
                      AppOutlinedIconButton(
                        text: 'Edit path',
                        icon: Icons.edit,
                        enabled: _enabled && mount.mounted == false,
                        onPressed: () async {
                          setState(() {
                            _enabled = false;
                          });

                          final available = await mount.getAvailableLocations();

                          if (!mounted) {
                            setState(() {
                              _enabled = true;
                            });
                            return;
                          }

                          if (!context.mounted) {
                            return;
                          }

                          final location = await editMountLocation(
                            context,
                            available,
                            location: mount.path,
                          );
                          if (location != null) {
                            await mount.setMountLocation(location);
                          }

                          if (mounted) {
                            setState(() {
                              _enabled = true;
                            });
                          }
                        },
                      ),
                      const SizedBox(width: constants.padding),
                      Expanded(
                        child: SelectableText(
                          _prettyPath(mount),
                          style: subStyle,
                        ),
                      ),
                      IntrinsicWidth(
                        child: Theme(
                          data: Theme.of(context).copyWith(
                            listTileTheme: const ListTileThemeData(
                              contentPadding: EdgeInsets.zero,
                              horizontalTitleGap: 0,
                              minLeadingWidth: 0,
                              minVerticalPadding: 0,
                            ),
                            checkboxTheme: CheckboxThemeData(
                              materialTapTargetSize:
                                  MaterialTapTargetSize.shrinkWrap,
                              visualDensity: visualDensity,
                            ),
                          ),
                          child: CheckboxListTile(
                            enabled:
                                !(mount.path.isEmpty && mount.mounted == null),
                            contentPadding: EdgeInsets.zero,
                            materialTapTargetSize:
                                MaterialTapTargetSize.shrinkWrap,
                            onChanged: (_) async {
                              return mount.setMountAutoStart(!mount.autoStart);
                            },
                            title: Row(
                              mainAxisSize: MainAxisSize.min,
                              children: const [
                                Icon(
                                  Icons.auto_mode,
                                  size: constants.smallIconSize,
                                ),
                                SizedBox(width: constants.paddingSmall),
                                Text('Auto-mount'),
                                SizedBox(width: constants.paddingSmall),
                              ],
                            ),
                            value: mount.autoStart,
                            visualDensity: visualDensity,
                          ),
                        ),
                      ),
                    ],
                  ),
                ],
              );
            },
          ),
        ),
      ),
    );
  }

  String _prettyPath(Mount mount) {
    if (mount.path.isEmpty && mount.mounted == null) {
      return 'loading...';
    }
    if (mount.path.isEmpty) {
      return '<mount location not set>';
    }
    return mount.path;
  }

  VoidCallback? _createMountHandler(BuildContext context, Mount mount) {
    if (!(_enabled && mount.mounted != null)) {
      return null;
    }

    return () async {
      if (mount.mounted == null) {
        return;
      }

      final mounted = mount.mounted!;
      setState(() {
        _enabled = false;
      });

      final location = await _getMountLocation(context, mount);

      void cleanup() {
        setState(() {
          _enabled = true;
        });
      }

      if (!mounted && location == null) {
        if (!context.mounted) {
          return;
        }
        displayErrorMessage(context, 'Mount location is not set');
        cleanup();
        return;
      }

      final success = await mount.mount(mounted, location: location);
      if (success ||
          mounted ||
          constants.navigatorKey.currentContext == null ||
          !constants.navigatorKey.currentContext!.mounted) {
        cleanup();
        return;
      }

      displayErrorMessage(
        context,
        'Mount location is not available: $location',
      );
      cleanup();
    };
  }

  Future<String?> _getMountLocation(BuildContext context, Mount mount) async {
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

    // ignore: use_build_context_synchronously
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
  void dispose() {
    _timer?.cancel();
    _timer = null;
    super.dispose();
  }
}
