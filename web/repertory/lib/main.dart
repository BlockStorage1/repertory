import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/screens/add_mount_screen.dart';
import 'package:repertory/screens/auth_screen.dart';
import 'package:repertory/screens/edit_mount_screen.dart';
import 'package:repertory/screens/edit_settings_screen.dart';
import 'package:repertory/screens/home_screen.dart';
import 'package:sodium_libs/sodium_libs.dart' show SodiumInit;

void main() async {
  try {
    constants.setSodium(await SodiumInit.init());
  } catch (e) {
    debugPrint('$e');
  }

  final auth = Auth();
  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => auth),
        ChangeNotifierProvider(create: (_) => MountList(auth)),
      ],
      child: const MyApp(),
    ),
  );
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  @override
  Widget build(context) {
    final snackBarTheme = SnackBarThemeData(
      width: MediaQuery.of(context).size.width * 0.50,
      behavior: SnackBarBehavior.floating,
    );

    return MaterialApp(
      navigatorKey: constants.navigatorKey,
      themeMode: ThemeMode.dark,
      darkTheme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.dark,
        colorScheme: ColorScheme.fromSeed(
          brightness: Brightness.dark,
          onSurface: Colors.white70,
          seedColor: Colors.deepOrange,
          surface: Color.fromARGB(255, 32, 33, 36),
          surfaceContainerLow: Color.fromARGB(255, 41, 42, 45),
        ),
        snackBarTheme: snackBarTheme,
      ),
      title: constants.appTitle,
      initialRoute: '/auth',
      routes: {
        '/':
            (context) =>
                const AuthCheck(child: HomeScreen(title: constants.appTitle)),
        '/add':
            (context) => const AuthCheck(
              child: AddMountScreen(title: constants.addMountTitle),
            ),
        '/auth': (context) => const AuthScreen(title: constants.appTitle),
        '/settings':
            (context) => const AuthCheck(
              child: EditSettingsScreen(title: constants.appSettingsTitle),
            ),
      },
      onGenerateRoute: (settings) {
        if (settings.name != '/edit') {
          return null;
        }

        final mount = settings.arguments as Mount;
        return MaterialPageRoute(
          builder: (context) {
            return AuthCheck(
              child: EditMountScreen(
                mount: mount,
                title:
                    '${mount.provider} [${formatMountName(mount.type, mount.name)}] Settings',
              ),
            );
          },
        );
      },
    );
  }
}

class AuthCheck extends StatelessWidget {
  final Widget child;
  const AuthCheck({super.key, required this.child});

  @override
  Widget build(BuildContext context) {
    return Consumer<Auth>(
      builder: (context, auth, __) {
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
