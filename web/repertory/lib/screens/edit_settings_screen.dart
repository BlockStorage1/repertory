import 'dart:convert' show jsonDecode, jsonEncode;

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:provider/provider.dart';
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/widgets/ui_settings.dart';

class EditSettingsScreen extends StatefulWidget {
  final String title;
  const EditSettingsScreen({super.key, required this.title});

  @override
  State<EditSettingsScreen> createState() => _EditSettingsScreenState();
}

class _EditSettingsScreenState extends State<EditSettingsScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
        actions: [
          Consumer<Auth>(
            builder: (context, auth, _) {
              return IconButton(
                icon: const Icon(Icons.logout),
                onPressed: () => auth.logoff(),
              );
            },
          ),
        ],
      ),
      body: FutureBuilder(
        builder: (context, snapshot) {
          if (!snapshot.hasData) {
            return Center(child: CircularProgressIndicator());
          }

          return UISettingsWidget(
            origSettings: jsonDecode(jsonEncode(snapshot.requireData)),
            settings: snapshot.requireData,
            showAdvanced: false,
          );
        },
        future: _grabSettings(),
        initialData: <String, dynamic>{},
      ),
    );
  }

  Future<Map<String, dynamic>> _grabSettings() async {
    try {
      final authProvider = Provider.of<Auth>(context, listen: false);
      final auth = await authProvider.createAuth();
      final response = await http.get(
        Uri.parse('${getBaseUri()}/api/v1/settings?auth=$auth'),
      );

      if (response.statusCode == 401) {
        authProvider.logoff();
        return {};
      }

      if (response.statusCode != 200) {
        return {};
      }

      return jsonDecode(response.body);
    } catch (e) {
      debugPrint('$e');
    }

    return {};
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
