import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/models/auth.dart';

class AuthScreen extends StatefulWidget {
  final String title;
  const AuthScreen({super.key, required this.title});

  @override
  State<AuthScreen> createState() => _AuthScreenState();
}

class _AuthScreenState extends State<AuthScreen> {
  bool _enabled = true;
  final _passwordController = TextEditingController();
  final _userController = TextEditingController();

  @override
  Widget build(context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
      ),
      body: Consumer<Auth>(
        builder: (context, auth, _) {
          if (auth.authenticated) {
            Future.delayed(Duration(milliseconds: 1), () {
              if (constants.navigatorKey.currentContext == null) {
                return;
              }

              Navigator.of(
                constants.navigatorKey.currentContext!,
              ).pushNamedAndRemoveUntil('/', (Route<dynamic> route) => false);
            });
            return SizedBox.shrink();
          }

          createLoginHandler() {
            return _enabled
                ? () async {
                  setState(() => _enabled = false);
                  await auth.authenticate(
                    _userController.text,
                    _passwordController.text,
                  );
                  setState(() => _enabled = true);
                }
                : null;
          }

          return Center(
            child: Card(
              child: Padding(
                padding: const EdgeInsets.all(constants.padding),
                child: SizedBox(
                  width: constants.logonWidth,
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    mainAxisAlignment: MainAxisAlignment.start,
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      Text(
                        constants.appLogonTitle,
                        textAlign: TextAlign.center,
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: constants.padding),
                      TextField(
                        autofocus: true,
                        decoration: InputDecoration(labelText: 'Username'),
                        controller: _userController,
                        textInputAction: TextInputAction.next,
                      ),
                      const SizedBox(height: constants.padding),
                      TextField(
                        obscureText: true,
                        decoration: InputDecoration(labelText: 'Password'),
                        controller: _passwordController,
                        textInputAction: TextInputAction.go,
                        onSubmitted: (_) {
                          final handler = createLoginHandler();
                          if (handler == null) {
                            return;
                          }

                          handler();
                        },
                      ),
                      const SizedBox(height: constants.padding),
                      ElevatedButton(
                        onPressed: createLoginHandler(),
                        child: const Text('Login'),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
