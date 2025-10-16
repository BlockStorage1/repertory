// section_card.dart

import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;

class SectionCard extends StatelessWidget {
  const SectionCard({super.key, required this.title, required this.child});
  final String title;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Card(
      elevation: 0,
      margin: const EdgeInsets.symmetric(vertical: constants.paddingSmall),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(constants.borderRadiusSmall),
      ),
      child: Padding(
        padding: const EdgeInsets.all(constants.padding),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: theme.textTheme.titleMedium),
            const SizedBox(height: constants.paddingSmall),
            child,
          ],
        ),
      ),
    );
  }
}
