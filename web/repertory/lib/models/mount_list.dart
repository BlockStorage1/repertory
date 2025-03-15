import 'dart:convert';

import 'package:collection/collection.dart';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/helpers.dart';
import 'package:repertory/types/mount_config.dart';

class MountList with ChangeNotifier {
  MountList() {
    _fetch();
  }

  List<MountConfig> _mountList = [];

  UnmodifiableListView<MountConfig> get items =>
      UnmodifiableListView<MountConfig>(_mountList);

  bool hasBucketName(String mountType, String bucket, {String? excludeName}) {
    final list = items.where(
      (item) => item.type.toLowerCase() == mountType.toLowerCase(),
    );

    return (excludeName == null
                ? list
                : list.whereNot(
                  (item) =>
                      item.name.toLowerCase() == excludeName.toLowerCase(),
                ))
            .firstWhereOrNull((MountConfig item) {
              return item.bucket != null &&
                  item.bucket!.toLowerCase() == bucket.toLowerCase();
            }) !=
        null;
  }

  bool hasConfigName(String name) {
    return items.firstWhereOrNull(
          (item) => item.name.toLowerCase() == name.toLowerCase(),
        ) !=
        null;
  }

  Future<void> _fetch() async {
    try {
      final response = await http.get(
        Uri.parse('${getBaseUri()}/api/v1/mount_list'),
      );

      if (response.statusCode != 200) {
        return;
      }
      List<MountConfig> nextList = [];

      jsonDecode(response.body).forEach((key, value) {
        nextList.addAll(
          value.map((name) => MountConfig.fromJson(key, name)).toList(),
        );
      });
      _sort(nextList);
      _mountList = nextList;

      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }
  }

  void _sort(list) {
    list.sort((a, b) {
      final res = a.type.compareTo(b.type);
      if (res != 0) {
        return res;
      }

      return a.name.compareTo(b.name);
    });
  }

  Future<void> add(
    String type,
    String name,
    Map<String, dynamic> mountConfig,
  ) async {
    try {
      await http.post(
        Uri.parse(
          Uri.encodeFull(
            '${getBaseUri()}/api/v1/add_mount?name=$name&type=$type&config=${jsonEncode(convertAllToString(mountConfig))}',
          ),
        ),
      );

      _mountList = [];
      notifyListeners();
    } catch (e) {
      debugPrint('$e');
    }

    return _fetch();
  }

  void remove(String name) {
    _mountList.removeWhere((item) => item.name == name);

    notifyListeners();
  }
}
