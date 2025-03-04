String formatMountName(String type, String name) {
  if (type == "remote") {
    return name.replaceAll("_", ":");
  }
  return name;
}

String initialCaps(String txt) {
  if (txt.isEmpty) {
    return txt;
  }

  if (txt.length == 1) {
    return txt[0].toUpperCase();
  }

  return txt[0].toUpperCase() + txt.substring(1).toLowerCase();
}
