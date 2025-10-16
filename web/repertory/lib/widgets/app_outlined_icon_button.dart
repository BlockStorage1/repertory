// app_outlined_icon_button.dart

import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;

class AppOutlinedIconButton extends StatelessWidget {
  final IconData icon;
  final bool enabled;
  final String text;
  final VoidCallback onPressed;

  const AppOutlinedIconButton({
    super.key,
    required this.icon,
    required this.enabled,
    required this.onPressed,
    required this.text,
  });

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    return Opacity(
      opacity: enabled ? 1.0 : constants.secondaryAlpha,
      child: OutlinedButton.icon(
        onPressed: enabled ? onPressed : null,
        icon: Icon(icon, size: constants.smallIconSize),
        label: Text(text),
        style: OutlinedButton.styleFrom(
          foregroundColor: scheme.primary,
          side: BorderSide(
            color: scheme.primary.withValues(alpha: constants.secondaryAlpha),
            width: 1.2,
          ),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(constants.borderRadiusSmall),
          ),
          padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
          backgroundColor: scheme.primary.withValues(
            alpha: constants.outlineAlpha,
          ),
        ),
      ),
    );
  }
}
