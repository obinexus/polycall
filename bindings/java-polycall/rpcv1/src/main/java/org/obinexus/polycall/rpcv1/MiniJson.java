package org.obinexus.polycall.rpcv1;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Minimal read-only JSON parser (the JDK has no built-in one). Enough to read
 * a polycall_rpc v1 RESPONSE / CONTROL_REPLY payload: objects, arrays,
 * strings, numbers, booleans, null. Mirrors src/config/json.c's scope.
 */
final class MiniJson {
    private final String s;
    private int i;

    private MiniJson(String s) {
        this.s = s;
    }

    /** Parses text; throws IllegalArgumentException on malformed JSON. */
    static Object parse(String text) {
        MiniJson p = new MiniJson(text);
        p.ws();
        Object v = p.value();
        p.ws();
        if (p.i != p.s.length()) {
            throw new IllegalArgumentException("trailing data at " + p.i);
        }
        return v;
    }

    /** obj.get(key) for a parsed Map, or null if not an object / absent. */
    @SuppressWarnings("unchecked")
    static Object get(Object obj, String key) {
        if (obj instanceof Map) {
            return ((Map<String, Object>) obj).get(key);
        }
        return null;
    }

    static String asString(Object v, String dflt) {
        return v instanceof String ? (String) v : dflt;
    }

    static boolean asBool(Object v, boolean dflt) {
        return v instanceof Boolean ? (Boolean) v : dflt;
    }

    private void ws() {
        while (i < s.length() && Character.isWhitespace(s.charAt(i))) i++;
    }

    private char peek() {
        if (i >= s.length()) throw new IllegalArgumentException("unexpected end");
        return s.charAt(i);
    }

    private Object value() {
        ws();
        char c = peek();
        switch (c) {
            case '{': return object();
            case '[': return array();
            case '"': return string();
            case 't': expect("true"); return Boolean.TRUE;
            case 'f': expect("false"); return Boolean.FALSE;
            case 'n': expect("null"); return null;
            default: return number();
        }
    }

    private void expect(String lit) {
        if (!s.startsWith(lit, i)) throw new IllegalArgumentException("bad literal at " + i);
        i += lit.length();
    }

    private Map<String, Object> object() {
        Map<String, Object> m = new LinkedHashMap<>();
        i++; // '{'
        ws();
        if (peek() == '}') { i++; return m; }
        while (true) {
            ws();
            String key = string();
            ws();
            if (peek() != ':') throw new IllegalArgumentException("expected ':' at " + i);
            i++;
            Object v = value();
            m.put(key, v);
            ws();
            char c = peek();
            if (c == ',') { i++; continue; }
            if (c == '}') { i++; break; }
            throw new IllegalArgumentException("expected ',' or '}' at " + i);
        }
        return m;
    }

    private List<Object> array() {
        List<Object> a = new ArrayList<>();
        i++; // '['
        ws();
        if (peek() == ']') { i++; return a; }
        while (true) {
            a.add(value());
            ws();
            char c = peek();
            if (c == ',') { i++; continue; }
            if (c == ']') { i++; break; }
            throw new IllegalArgumentException("expected ',' or ']' at " + i);
        }
        return a;
    }

    private String string() {
        if (peek() != '"') throw new IllegalArgumentException("expected string at " + i);
        i++;
        StringBuilder sb = new StringBuilder();
        while (true) {
            char c = s.charAt(i++);
            if (c == '"') break;
            if (c == '\\') {
                char e = s.charAt(i++);
                switch (e) {
                    case '"': sb.append('"'); break;
                    case '\\': sb.append('\\'); break;
                    case '/': sb.append('/'); break;
                    case 'b': sb.append('\b'); break;
                    case 'f': sb.append('\f'); break;
                    case 'n': sb.append('\n'); break;
                    case 'r': sb.append('\r'); break;
                    case 't': sb.append('\t'); break;
                    case 'u':
                        sb.append((char) Integer.parseInt(s.substring(i, i + 4), 16));
                        i += 4;
                        break;
                    default: throw new IllegalArgumentException("bad escape at " + i);
                }
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    private Double number() {
        int start = i;
        if (peek() == '-') i++;
        while (i < s.length() && Character.isDigit(s.charAt(i))) i++;
        if (i < s.length() && s.charAt(i) == '.') {
            i++;
            while (i < s.length() && Character.isDigit(s.charAt(i))) i++;
        }
        if (i < s.length() && (s.charAt(i) == 'e' || s.charAt(i) == 'E')) {
            i++;
            if (i < s.length() && (s.charAt(i) == '+' || s.charAt(i) == '-')) i++;
            while (i < s.length() && Character.isDigit(s.charAt(i))) i++;
        }
        if (i == start) throw new IllegalArgumentException("bad number at " + i);
        return Double.parseDouble(s.substring(start, i));
    }
}
