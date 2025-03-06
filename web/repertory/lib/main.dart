import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/screens/add_mount_screen.dart';
import 'package:repertory/screens/edit_mount_screen.dart';
import 'package:repertory/screens/home_screen.dart';

void main() {
  runApp(
    ChangeNotifierProvider(create: (_) => MountList(), child: const MyApp()),
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
    return MaterialApp(
      title: constants.appTitle,
      theme: ThemeData(
        brightness: Brightness.light,
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.deepOrange,
          brightness: Brightness.light,
        ),
      ),
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
      ),
      initialRoute: '/',
      routes: {
        '/': (context) => const HomeScreen(title: constants.appTitle),
        '/add':
            (context) => const AddMountScreen(title: constants.addMountTitle),
      },
      onGenerateRoute: (settings) {
        if (settings.name != '/settings') {
          return null;
        }

        final mount = settings.arguments as Mount;
        return MaterialPageRoute(
          builder: (context) {
            return EditMountScreen(
              mount: mount,
              title:
                  '${initialCaps(mount.type)} [${formatMountName(mount.type, mount.name)}] Settings',
            );
          },
        );
      },
    );
  }
}
