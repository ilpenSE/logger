import com.sun.jna.Pointer;

public class Main {
  static LoggerLib.LogFormatter formatter;

  public static void main(String[] args) {
    LoggerLib lg = LoggerLib.INSTANCE;

    formatter = (time_str, level, msg, pack) -> {
      pack.read();
      String lvl = LoggerLib.INSTANCE.lg_lvl_to_str(level);
      String formatted = String.format("%s %s: %s\n", time_str, lvl, msg);

      if (pack.stdout_str.data != null && pack.stdout_str.cap.longValue() != 0) {
        lg.lg_str_write_into(pack.stdout_str, formatted);
      }

      if (pack.file_str.data != null && pack.file_str.cap.longValue() != 0) {
        lg.lg_str_write_into(pack.file_str, formatted);
      }

      pack.write();
    };

    Pointer inst = lg.lg_alloc();
    if (inst == null) {
      System.err.println("lg_alloc failed");
      return;
    }

    int ok = lg.lg_init_flat(inst, "logs", 1, 1, LoggerLib.LG_DROP, formatter);
    if (ok == 0) {
      System.err.println("lg_init_flat failed");
      lg.lg_free(inst);
      return;
    }

    lg.lg_finfo("Hello from Java!");
    lg.lg_fwarn("This is a warning");
    lg.lg_ferror("This is an error");

    lg.lg_destroy(inst);
    lg.lg_free(inst);
  }
}
