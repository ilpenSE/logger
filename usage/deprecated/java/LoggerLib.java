import java.util.Arrays;
import java.util.List;
import com.sun.jna.*;
import com.sun.jna.ptr.*;

public interface LoggerLib extends Library {
  LoggerLib INSTANCE = Native.load("logger", LoggerLib.class);

  // Log Level Enum
  int LG_INFO    = 1 << 0;
  int LG_ERROR   = 1 << 1;
  int LG_WARNING = 1 << 2;
  int LG_CUSTOM  = 1 << 3;

  // Policy enum
  int LG_DROP         = 1 << 0;
  int LG_SMASH_OLDEST = 1 << 1;

  // Memory allocators
  Pointer lg_alloc();
  void lg_free(Pointer inst);

  // Lifetime managers
  int lg_init_flat(Pointer inst, String logs_dir,
                   int local_time, int print_stdout,
                   int policy, LogFormatter log_formatter);
  int lg_destroy(Pointer inst);
  Pointer lg_get_active_instance();
  int lg_set_active_instance(Pointer inst);

  // Logger functions
  int lg_flog(int level, String msg);
  int lg_finfo(String msg);
  int lg_fwarn(String msg);
  int lg_ferror(String msg);

  int lg_flogi(Pointer inst, int level, String msg);
  int lg_finfoi(Pointer inst, String msg);
  int lg_fwarni(Pointer inst, String msg);
  int lg_ferrori(Pointer inst, String msg);

  // Helpers
  int lg_get_time_str(byte[] time_str, int isLocalTime);
  String lg_lvl_to_str(int level);
  void lg_str_write_into(LgString s, String already_formatted_str);

  class LgMsgPack extends Structure {
    public static class ByReference extends LgMsgPack implements Structure.ByReference {}

    public LgString stdout_str;
    public LgString file_str;

    @Override
    protected List<String> getFieldOrder() {
      return Arrays.asList("stdout_str", "file_str");
    }
  }

  class LgString extends Structure {
    public static class ByReference extends LgString implements Structure.ByReference {}

    public Pointer data;
    public NativeLong cap;
    public NativeLong len;

    @Override
    protected List<String> getFieldOrder() {
      return Arrays.asList("data", "cap", "len");
    }
  }

  interface LogFormatter extends Callback {
    int invoke(int isLocalTime, int level, String msg, LgMsgPack.ByReference pack);
  }
}
