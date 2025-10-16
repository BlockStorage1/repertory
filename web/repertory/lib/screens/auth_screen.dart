// auth_screen.dart

import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/settings.dart';
import 'package:repertory/widgets/app_text_field.dart';
import 'package:repertory/widgets/aurora_sweep.dart';

class AuthScreen extends StatefulWidget {
  final String title;
  const AuthScreen({super.key, required this.title});

  @override
  State<AuthScreen> createState() => _AuthScreenState();
}

class _AuthScreenState extends State<AuthScreen> {
  final _formKey = GlobalKey<FormState>();
  final _passwordController = TextEditingController();
  final _userController = TextEditingController();

  bool _enabled = true;
  bool _obscure = true;

  @override
  void dispose() {
    _passwordController.dispose();
    _userController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    Future<void> doLogin(Auth auth) async {
      if (!_enabled) {
        return;
      }
      if (!_formKey.currentState!.validate()) {
        return;
      }

      setState(() => _enabled = false);
      final authenticated = await auth.authenticate(
        _userController.text.trim(),
        _passwordController.text,
      );
      setState(() => _enabled = true);

      if (authenticated) {
        return;
      }

      if (!context.mounted) {
        return;
      }

      displayErrorMessage(context, 'Invalid username or password', clear: true);
    }

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
            Align(
              alignment: const Alignment(0, 0.06),
              child: IgnorePointer(
                child: Container(
                  width: 720,
                  height: 720,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: RadialGradient(
                      colors: [
                        scheme.primary.withValues(
                          alpha: constants.primaryAlpha,
                        ),
                        Colors.transparent,
                      ],
                      stops: const [0.0, 1.0],
                    ),
                  ),
                ),
              ),
            ),
            Consumer<Auth>(
              builder: (context, auth, _) {
                if (auth.authenticated) {
                  WidgetsBinding.instance.addPostFrameCallback((_) {
                    if (constants.navigatorKey.currentContext == null) {
                      return;
                    }
                    Navigator.of(
                      constants.navigatorKey.currentContext!,
                    ).pushNamedAndRemoveUntil('/', (r) => false);
                  });
                  return const SizedBox.shrink();
                }

                return Center(
                  child: AnimatedScale(
                    scale: 1.0,
                    duration: const Duration(milliseconds: 250),
                    curve: Curves.easeOutCubic,
                    child: AnimatedOpacity(
                      opacity: 1.0,
                      duration: const Duration(milliseconds: 250),
                      child: ConstrainedBox(
                        constraints: const BoxConstraints(
                          maxWidth: constants.logonWidth,
                          minWidth: constants.logonWidth,
                        ),
                        child: Card(
                          elevation: constants.padding,
                          color: scheme.primary.withValues(
                            alpha: constants.outlineAlpha,
                          ),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(
                              constants.borderRadius,
                            ),
                          ),
                          child: Padding(
                            padding: const EdgeInsets.all(
                              constants.paddingLarge,
                            ),
                            child: Form(
                              key: _formKey,
                              autovalidateMode:
                                  AutovalidateMode.onUserInteraction,
                              child: Column(
                                mainAxisSize: MainAxisSize.min,
                                crossAxisAlignment: CrossAxisAlignment.stretch,
                                children: [
                                  Align(
                                    alignment: Alignment.center,
                                    child: Container(
                                      width:
                                          constants.loginIconSize +
                                          constants.paddingMedium * 2.0,
                                      height:
                                          constants.loginIconSize +
                                          constants.paddingMedium * 2.0,
                                      decoration: BoxDecoration(
                                        color: scheme.primary.withValues(
                                          alpha: constants.outlineAlpha,
                                        ),
                                        borderRadius: BorderRadius.circular(
                                          constants.borderRadiusSmall,
                                        ),
                                        boxShadow: [
                                          BoxShadow(
                                            color: Colors.black.withValues(
                                              alpha: constants.boxShadowAlpha,
                                            ),
                                            blurRadius: constants.borderRadius,
                                            offset: Offset(
                                              0,
                                              constants.borderRadius,
                                            ),
                                          ),
                                        ],
                                      ),
                                      padding: const EdgeInsets.all(
                                        constants.paddingMedium,
                                      ),
                                      child: Image.asset(
                                        'assets/images/repertory.png',
                                        fit: BoxFit.contain,
                                        errorBuilder: (_, _, _) {
                                          return Icon(
                                            Icons.folder,
                                            color: scheme.primary,
                                            size: constants.loginIconSize,
                                          );
                                        },
                                      ),
                                    ),
                                  ),
                                  const SizedBox(
                                    height: constants.paddingSmall,
                                  ),
                                  Text(
                                    constants.appLogonTitle,
                                    textAlign: TextAlign.center,
                                    style: Theme.of(context)
                                        .textTheme
                                        .headlineSmall
                                        ?.copyWith(fontWeight: FontWeight.w600),
                                  ),
                                  const SizedBox(
                                    height: constants.paddingSmall,
                                  ),
                                  Text(
                                    "Secure access to your mounts",
                                    textAlign: TextAlign.center,
                                    style: Theme.of(context)
                                        .textTheme
                                        .bodyMedium
                                        ?.copyWith(color: scheme.onSurface),
                                  ),
                                  const SizedBox(
                                    height: constants.paddingLarge,
                                  ),
                                  AppTextField(
                                    autofocus: true,
                                    controller: _userController,
                                    icon: Icons.person,
                                    labelText: 'Username',
                                    textInputAction: TextInputAction.next,
                                    validator: (v) {
                                      if (v == null || v.trim().isEmpty) {
                                        return 'Enter your username';
                                      }
                                      return null;
                                    },
                                    onFieldSubmitted: (_) {
                                      FocusScope.of(context).nextFocus();
                                    },
                                  ),
                                  const SizedBox(height: constants.padding),
                                  AppTextField(
                                    controller: _passwordController,
                                    icon: Icons.lock,
                                    labelText: 'Password',
                                    obscureText: _obscure,
                                    suffixIcon: IconButton(
                                      tooltip: _obscure
                                          ? 'Show password'
                                          : 'Hide password',
                                      icon: Icon(
                                        _obscure
                                            ? Icons.visibility
                                            : Icons.visibility_off,
                                      ),
                                      onPressed: () {
                                        setState(() {
                                          _obscure = !_obscure;
                                        });
                                      },
                                    ),
                                    textInputAction: TextInputAction.go,
                                    validator: (v) {
                                      if (v == null || v.isEmpty) {
                                        return 'Enter your password';
                                      }
                                      return null;
                                    },
                                    onFieldSubmitted: (_) {
                                      doLogin(auth);
                                    },
                                  ),
                                  const SizedBox(height: constants.padding),
                                  SizedBox(
                                    height: 46,
                                    child: ElevatedButton(
                                      onPressed: _enabled
                                          ? () {
                                              doLogin(auth);
                                            }
                                          : null,
                                      style: ElevatedButton.styleFrom(
                                        backgroundColor: scheme.primary
                                            .withValues(
                                              alpha: constants.secondaryAlpha,
                                            ),
                                        disabledBackgroundColor: scheme.primary
                                            .withValues(
                                              alpha: constants.outlineAlpha,
                                            ),
                                        shape: RoundedRectangleBorder(
                                          borderRadius: BorderRadius.circular(
                                            constants.borderRadiusSmall,
                                          ),
                                        ),
                                      ),
                                      child: _enabled
                                          ? const Text('Login')
                                          : const SizedBox(
                                              height: 20,
                                              width: 20,
                                              child: CircularProgressIndicator(
                                                strokeWidth: 2.4,
                                              ),
                                            ),
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ),
                        ),
                      ),
                    ),
                  ),
                );
              },
            ),
          ],
        ),
      ),
    );
  }
}
