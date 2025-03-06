import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/widgets/add_mount_widget.dart';
import 'package:repertory/widgets/mount_list_widget.dart';
import 'package:repertory/widgets/mount_settings.dart';

void main() {
  runApp(
    ChangeNotifierProvider(create: (_) => MountList(), child: const MyApp()),
  );
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

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
      routes: {'/': (context) => const MyHomePage(title: constants.appTitle)},
      onGenerateRoute: (settings) {
        if (settings.name != '/settings') {
          return null;
        }

        final mount = settings.arguments as Mount;
        return MaterialPageRoute(
          builder: (context) {
            return Scaffold(
              appBar: AppBar(
                backgroundColor: Theme.of(context).colorScheme.inversePrimary,
                title: Text(
                  '${initialCaps(mount.type)} [${formatMountName(mount.type, mount.name)}] Settings',
                ),
              ),
              body: MountSettingsWidget(mount: mount),
            );
          },
        );
      },
    );
  }
}

class MyHomePage extends StatefulWidget {
  final String title;
  const MyHomePage({super.key, required this.title});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  bool _allowAdd = true;
  String? _apiPassword;
  String? _apiPort;
  String? _bucket;
  String? _encryptionToken;
  String? _hostNameOrIp;
  String _mountType = 'Encrypt';
  String _mountName = "";
  String? _path;

  void _resetData() {
    _apiPassword = null;
    _apiPort = null;
    _bucket = null;
    _encryptionToken = null;
    _hostNameOrIp = null;
    _mountName = "";
    _mountType = 'Encrypt';
    _path = null;
  }

  void _updateData(String name, String? value) {
    switch (name) {
      case 'ApiPassword':
        _apiPassword = value ?? '';
        return;

      case 'ApiPort':
        _apiPort = value ?? '';
        return;

      case 'Bucket':
        _bucket = value ?? '';
        return;

      case 'EncryptionToken':
        _encryptionToken = value ?? '';
        return;

      case 'HostNameOrIp':
        _hostNameOrIp = value ?? '';
        return;

      case 'Name':
        _mountName = value ?? '';
        return;

      case 'Provider':
        _mountType = value ?? 'Encrypt';
        return;

      case 'Path':
        _path = value ?? '';
        return;
    }
  }

  @override
  Widget build(context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        leading: const Icon(Icons.storage),
        title: Text(widget.title),
      ),
      body: MountListWidget(),
      floatingActionButton: FloatingActionButton(
        onPressed:
            _allowAdd
                ? () async => showDialog(
                  context: context,
                  builder: (context) {
                    return AlertDialog(
                      title: const Text('Add Mount'),
                      content: Consumer<MountList>(
                        builder: (_, MountList mountList, __) {
                          return AddMountWidget(
                            mountType: _mountType,
                            onDataChanged: _updateData,
                          );
                        },
                      ),
                      actions: [
                        TextButton(
                          child: const Text('Cancel'),
                          onPressed: () {
                            _resetData();
                            Navigator.of(context).pop();
                          },
                        ),
                        TextButton(
                          child: const Text('OK'),
                          onPressed: () {
                            setState(() => _allowAdd = false);

                            Provider.of<MountList>(context, listen: false)
                                .add(
                                  _mountType,
                                  _mountName,
                                  apiPassword: _apiPassword,
                                  apiPort: _apiPort,
                                  bucket: _bucket,
                                  encryptionToken: _encryptionToken,
                                  hostNameOrIp: _hostNameOrIp,
                                  path: _path,
                                )
                                .then((_) {
                                  _resetData();
                                  setState(() {
                                    _allowAdd = true;
                                  });
                                })
                                .catchError((_) {
                                  setState(() => _allowAdd = true);
                                });

                            Navigator.of(context).pop();
                          },
                        ),
                      ],
                    );
                  },
                )
                : null,
        tooltip: 'Add Mount',
        child: const Icon(Icons.add),
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
