// home_screen.dart

import 'package:flutter/material.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/widgets/app_scaffold.dart';
import 'package:repertory/widgets/mount_list_widget.dart';

class HomeScreen extends StatefulWidget {
  final String title;
  const HomeScreen({super.key, required this.title});

  @override
  State<HomeScreen> createState() => _HomeScreeState();
}

class _HomeScreeState extends State<HomeScreen> {
  @override
  Widget build(context) {
    final scheme = Theme.of(context).colorScheme;

    return AppScaffold(
      title: widget.title,
      floatingActionButton: Padding(
        padding: const EdgeInsets.all(constants.padding),
        child: Hero(
          tag: 'add_mount_fab',
          child: Material(
            color: scheme.primary.withValues(alpha: constants.secondaryAlpha),
            elevation: 12,
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(constants.borderRadius),
            ),
            child: Ink(
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(constants.borderRadius),
                border: Border.all(
                  color: scheme.outlineVariant.withValues(
                    alpha: constants.outlineAlpha,
                  ),
                  width: 1,
                ),
                gradient: const LinearGradient(
                  begin: Alignment.topCenter,
                  end: Alignment.bottomCenter,
                  colors: constants.gradientColors2,
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(
                      alpha: constants.boxShadowAlpha,
                    ),
                    blurRadius: constants.borderRadius,
                    offset: Offset(0, constants.borderRadius),
                  ),
                ],
              ),
              child: InkWell(
                borderRadius: BorderRadius.circular(constants.borderRadius),
                onTap: () {
                  Navigator.pushNamed(context, '/add');
                },
                child: const SizedBox(
                  width: 56,
                  height: 56,
                  child: Center(
                    child: Icon(Icons.add, size: constants.largeIconSize),
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
      children: [Expanded(child: const MountListWidget())],
    );
  }
}
