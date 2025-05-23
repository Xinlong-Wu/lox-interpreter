package JLox.Interpreter;

import java.util.HashMap;
import java.util.Map;

import JLox.Error.RuntimeError;
import JLox.Token.Token;

public class LoxInstance {
    private LoxClass klass;
    protected final Map<String, Object> fields = new HashMap<>();

    LoxInstance(LoxClass klass) {
        this.klass = klass;
    }

    Object get(Token name) {
        if (fields.containsKey(name.lexeme)) {
            return fields.get(name.lexeme);
        }

        LoxFunction method = klass.findMethod(name.lexeme);
        if (method != null)
            return method.bind(this);

        return method;
    }

    LoxFunction getter(Token name) {
        LoxFunction getter = klass.findGetter(name.lexeme);
        if (getter != null)
            return getter.bind(this);

        return null;
    }

    void set(Token name, Object value) {
        fields.put(name.lexeme, value);
    }

    @Override
    public String toString() {
        return klass.name + " instance";
    }
}