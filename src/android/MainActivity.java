package com.google.bigwheels;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import com.google.androidgamesdk.GameActivity;

public class MainActivity extends GameActivity {
  static { System.loadLibrary(BuildConfig.sample_library_name); }

  private String[] getArgsFromIntent(Intent intent) {
    String argString = intent.getStringExtra("arguments");
    if (argString != null) {
      return argString.split("\\s+");
    }
    return new String[0];
  }

  public String[] constructCmdLineArgs() {
    String[] intentArgs = getArgsFromIntent(getIntent());
    String[] args = new String[intentArgs.length + 1];
    // The first argument is the application name, and is ignored by the parser.
    args[0] = getTitle().toString();
    System.arraycopy(intentArgs, 0, args, 1, intentArgs.length);
    return args;
  }
}
