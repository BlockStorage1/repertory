// main.dart

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/models/settings.dart';
import 'package:repertory/screens/add_mount_screen.dart';
import 'package:repertory/screens/auth_screen.dart';
import 'package:repertory/screens/edit_mount_screen.dart';
import 'package:repertory/screens/edit_settings_screen.dart';
import 'package:repertory/screens/home_screen.dart';
import 'package:repertory/widgets/auth_check.dart';
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
        ChangeNotifierProvider(create: (_) => Settings(auth)),
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
          seedColor: constants.accentBlue,
          onSurface: Colors.white70,
          surface: constants.surfaceDark,
          surfaceContainerLow: constants.surfaceContainerLowDark,
        ),
        scaffoldBackgroundColor: constants.surfaceDark,
        snackBarTheme: snackBarTheme,
        appBarTheme: const AppBarTheme(scrolledUnderElevation: 0),
        inputDecorationTheme: const InputDecorationTheme(
          focusedBorder: UnderlineInputBorder(borderSide: BorderSide(width: 2)),
        ),
        elevatedButtonTheme: ElevatedButtonThemeData(
          style: ElevatedButton.styleFrom(
            minimumSize: const Size.fromHeight(40),
          ),
        ),
        filledButtonTheme: FilledButtonThemeData(
          style: FilledButton.styleFrom(minimumSize: const Size.fromHeight(40)),
        ),
        dividerTheme: const DividerThemeData(thickness: 0.6, space: 0),
      ),
      title: constants.appTitle,
      initialRoute: '/auth',
      routes: {
        '/': (context) =>
            const AuthCheck(child: HomeScreen(title: constants.appTitle)),
        '/add': (context) => const AuthCheck(
          child: AddMountScreen(title: constants.addMountTitle),
        ),
        '/auth': (context) => const AuthScreen(title: constants.appTitle),
        '/settings': (context) => const AuthCheck(
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
                    '${mount.provider} Settings • ${formatMountName(mount.type, mount.name)}',
              ),
            );
          },
        );
      },
    );
  }
}
