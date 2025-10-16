import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/settings.dart';
import 'package:repertory/widgets/aurora_sweep.dart';

class AppScaffold extends StatelessWidget {
  const AppScaffold({
    super.key,
    required this.children,
    required this.title,
    this.advancedWidget,
    this.floatingActionButton,
    this.showBack = false,
    this.showUISettings = false,
  });

  final List<Widget> children;
  final String title;
  final Widget? advancedWidget;
  final Widget? floatingActionButton;
  final bool showBack;
  final bool showUISettings;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final textTheme = Theme.of(context).textTheme;

    return Scaffold(
      body: SafeArea(
        child: Stack(
          children: [
            Container(
              width: double.infinity,
              height: double.infinity,
              decoration: const BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                  colors: constants.gradientColors,
                  stops: [0.0, 1.0],
                ),
              ),
            ),
            Consumer<Settings>(
              builder: (_, settings, _) =>
                  AuroraSweep(enabled: settings.enableAnimations),
            ),
            Positioned.fill(
              child: BackdropFilter(
                filter: ImageFilter.blur(sigmaX: 6, sigmaY: 6),
                child: Container(
                  color: Colors.black.withValues(alpha: constants.outlineAlpha),
                ),
              ),
            ),
            Padding(
              padding: const EdgeInsets.all(constants.padding),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Row(
                    children: [
                      if (!showBack) ...[
                        SizedBox(
                          width: 40,
                          height: 40,
                          child: Image.asset(
                            'assets/images/repertory.png',
                            fit: BoxFit.contain,
                            errorBuilder: (_, _, _) {
                              return Icon(
                                Icons.folder,
                                color: scheme.primary,
                                size: constants.largeIconSize,
                              );
                            },
                          ),
                        ),
                        const SizedBox(width: constants.padding),
                      ],
                      if (showBack) ...[
                        Material(
                          color: Colors.transparent,
                          child: InkWell(
                            borderRadius: BorderRadius.circular(
                              constants.borderRadius,
                            ),
                            onTap: () => Navigator.of(context).pop(),
                            child: Ink(
                              width: 40,
                              height: 40,
                              decoration: BoxDecoration(
                                color: scheme.surface.withValues(
                                  alpha: constants.secondaryAlpha,
                                ),
                                borderRadius: BorderRadius.circular(
                                  constants.borderRadius,
                                ),
                                border: Border.all(
                                  color: scheme.outlineVariant.withValues(
                                    alpha: constants.highlightAlpha,
                                  ),
                                  width: 1,
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
                              child: const Icon(Icons.arrow_back),
                            ),
                          ),
                        ),
                        const SizedBox(width: constants.padding),
                      ],
                      Expanded(
                        child: Text(
                          title,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: textTheme.headlineSmall?.copyWith(
                            fontWeight: FontWeight.w700,
                            color: scheme.onSurface,
                          ),
                        ),
                      ),
                      const SizedBox(width: constants.padding),
                      if (!showBack || showUISettings) ...[
                        const Text("Animations"),
                        Consumer<Settings>(
                          builder: (context, settings, _) => IconButton(
                            icon: Icon(
                              settings.enableAnimations
                                  ? Icons.toggle_on
                                  : Icons.toggle_off,
                            ),
                            color: settings.enableAnimations
                                ? scheme.primary
                                : scheme.onSurface,
                            onPressed: () => settings.setEnableAnimations(
                              !settings.enableAnimations,
                            ),
                          ),
                        ),
                        const Text("Auto-start"),
                        Consumer<Settings>(
                          builder: (context, settings, _) => IconButton(
                            icon: Icon(
                              settings.autoStart
                                  ? Icons.toggle_on
                                  : Icons.toggle_off,
                            ),
                            color: settings.autoStart
                                ? scheme.primary
                                : scheme.onSurface,
                            onPressed: () =>
                                settings.setAutoStart(!settings.autoStart),
                          ),
                        ),
                        if (showUISettings)
                          const SizedBox(width: constants.padding),
                      ],
                      if (!showBack) ...[
                        IconButton(
                          tooltip: 'Settings',
                          icon: const Icon(Icons.settings),
                          onPressed: () {
                            Navigator.pushNamed(context, '/settings');
                          },
                        ),
                        const SizedBox(width: constants.padding),
                      ],
                      if (showBack && advancedWidget != null) ...[
                        advancedWidget!,
                        const SizedBox(width: constants.padding),
                      ],
                      Consumer<Auth>(
                        builder: (context, auth, _) => IconButton(
                          tooltip: 'Log out',
                          icon: const Icon(Icons.logout),
                          onPressed: auth.logoff,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: constants.padding),
                  ...children,
                ],
              ),
            ),
          ],
        ),
      ),
      floatingActionButton: floatingActionButton,
    );
  }
}
