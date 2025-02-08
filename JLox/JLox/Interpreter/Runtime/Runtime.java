package JLox.Interpreter.Runtime;

import JLox.Interpreter.Environment;
import JLox.Interpreter.Interpreter;
import JLox.Interpreter.LoxCallable;

public class Runtime {
    public static void init(Environment environment) {
        // Define the standard library functions
        environment.define("clock", new LoxCallable() {
            @Override
            public int arity() {
                return 0;
            }

            @Override
            public Object call(Interpreter interpreter, java.util.List<Object> arguments) {
                return (double) System.currentTimeMillis() / 1000.0;
            }

            @Override
            public String toString() {
                return "<native fn>";
            }
        });
    }
}
