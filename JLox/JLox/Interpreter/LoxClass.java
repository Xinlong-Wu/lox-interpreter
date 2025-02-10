package JLox.Interpreter;

import java.util.List;
import java.util.Map;

public class LoxClass extends LoxInstance implements LoxCallable {
    final String name;
    final LoxClass superclass;
    private final Map<String, LoxFunction> methods, getters;

    LoxClass(String name, LoxClass superclass, Map<String, LoxFunction> staticMethods, Map<String, LoxFunction> methods,
            Map<String, LoxFunction> getters) {
        super(null);
        this.superclass = superclass;
        this.fields.putAll(staticMethods);
        this.name = name;
        this.methods = methods;
        this.getters = getters;
    }

    LoxFunction findMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

        if (superclass != null) {
            return superclass.findMethod(name);
        }

        return null;
    }

    LoxFunction findGetter(String name) {
        if (getters.containsKey(name)) {
            return getters.get(name);
        }

        if (superclass != null) {
            return superclass.findGetter(name);
        }

        return null;
    }

    @Override
    public String toString() {
        return name;
    }

    @Override
    public int arity() {
        LoxFunction initializer = findMethod("init");
        if (initializer == null)
            return 0;
        return initializer.arity();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod("init");
        if (initializer != null) {
            initializer.bind(instance).call(interpreter, arguments);
        }
        return instance;
    }
}