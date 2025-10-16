// auth_check.dart

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/models/auth.dart';

class AuthCheck extends StatelessWidget {
  final Widget child;
  const AuthCheck({super.key, required this.child});

  @override
  Widget build(BuildContext context) {
    return Consumer<Auth>(
      builder: (context, auth, _) {
        if (!auth.authenticated) {
          Future.delayed(Duration(milliseconds: 1), () {
            if (constants.navigatorKey.currentContext == null) {
              return;
            }
            Navigator.of(
              constants.navigatorKey.currentContext!,
            ).pushNamedAndRemoveUntil('/auth', (Route<dynamic> route) => false);
          });
          return child;
        }

        return child;
      },
    );
  }
}
