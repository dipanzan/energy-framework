package com.dipanzan;

public class CompleteSTE {
  public String clazz, method, args_types[], args[];

  public CompleteSTE() {
    this.clazz = "";
    this.method = "";
    this.args_types = null;
    this.args = null;
  }

  public CompleteSTE(String _clazz, String _method, String[] _args_types, String[] _args) {
    this.clazz = _clazz;
    this.method = _method;
    this.args_types = _args_types;
    this.args = _args;
  }

  @Override
  public String toString() {
    String ret = "\n";
    ret += "Class: " + this.clazz + "\n";
    ret += "Method: " + this.method + "\n";
    ret += "Arguments:\n";
    for (int i = 0; i < this.args_types.length; i++) {
      ret += "  * " + this.args_types[i] + " " + this.args[i] + "\n";
    }
    return ret;
  }
}
