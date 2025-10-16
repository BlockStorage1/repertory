// app_toggle_button_framed.dart

import 'package:flutter/material.dart';
import 'package:repertory/widgets/app_icon_button_framed.dart';

class AppToggleButtonFramed extends StatelessWidget {
  final bool? mounted;
  final VoidCallback? onPressed;

  const AppToggleButtonFramed({
    super.key,
    required this.mounted,
    required this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final bool isOn = mounted ?? false;

    IconData icon = Icons.hourglass_top;
    Color iconColor = scheme.onSurface;

    if (mounted != null) {
      icon = isOn ? Icons.toggle_on : Icons.toggle_off;
      iconColor = isOn ? scheme.primary : scheme.onSurface;
    }

    return AppIconButtonFramed(
      icon: icon,
      iconColor: iconColor,
      onPressed: mounted == null ? null : onPressed,
    );
  }
}
