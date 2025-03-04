class DuplicateMountException implements Exception {
  final String _name;
  const DuplicateMountException({required name}) : _name = name, super();

  String get name => _name;
}
