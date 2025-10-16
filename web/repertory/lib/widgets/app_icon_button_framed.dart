// app_icon_button_framed.dart

import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;

class AppIconButtonFramed extends StatelessWidget {
  final IconData icon;
  final VoidCallback? onPressed;
  final Color? iconColor;

  const AppIconButtonFramed({
    super.key,
    required this.icon,
    this.onPressed,
    this.iconColor,
  });

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final radius = BorderRadius.circular(constants.borderRadiusSmall);

    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: onPressed,
        borderRadius: radius,
        child: Ink(
          width: 46,
          height: 46,
          decoration: BoxDecoration(
            color: scheme.primary.withValues(alpha: constants.outlineAlpha),
            borderRadius: radius,
            border: Border.all(
              color: scheme.outlineVariant.withValues(
                alpha: constants.outlineAlpha,
              ),
              width: 1,
            ),
            boxShadow: [
              BoxShadow(
                color: Colors.black.withValues(alpha: constants.boxShadowAlpha),
                blurRadius: constants.borderRadiusTiny,
                offset: const Offset(0, constants.borderRadiusSmall),
              ),
            ],
          ),
          child: Center(
            child: Transform.scale(
              scale: 0.90,
              child: Icon(
                icon,
                color: iconColor ?? scheme.onSurface,
                size: constants.largeIconSize,
              ),
            ),
          ),
        ),
      ),
    );
  }
}
