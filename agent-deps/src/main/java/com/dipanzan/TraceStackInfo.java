package com.dipanzan;

import java.util.HashMap;
import java.util.Map;
import java.util.Stack;

public class TraceStackInfo {
  public final static HashMap<String, Stack<CompleteSTE>> STACKS = new HashMap<>();

  public static String callStackToJSON() {
    String ret = "{";
    for (Map.Entry<String, Stack<CompleteSTE>> entry : STACKS.entrySet()) {
      String key = entry.getKey();
      Stack<CompleteSTE> stk = entry.getValue();

      ret += "\"" + key + "\" : [";
      for (int i = 0; i < stk.size(); i++) {
        ret += "{";
        ret += "\"class\" : \"" + stk.get(i).clazz + "\", ";
        ret += "\"method\" : \"" + stk.get(i).method + "\", ";

        ret += "\"args_types\" : [";
        for (int j = 0; j < stk.get(i).args_types.length; j++) {
          ret += "\"" + stk.get(i).args_types[j] + "\"";
          if (j != stk.get(i).args_types.length - 1)
            ret += ", ";
        }
        ret += "], ";

        ret += "\"args\" : [";
        for (int j = 0; j < stk.get(i).args.length; j++) {
          ret += "\"" + stk.get(i).args[j] + "\"";
          if (j != stk.get(i).args.length - 1)
            ret += ", ";
        }
        ret += "]";
        ret += "}";
        if (i != stk.size() - 1)
          ret += ", ";
      }
      ret += "], ";
    }
    ret = ret.substring(0, ret.length() - 2);
    ret += "}";
    return ret;
  }

}
