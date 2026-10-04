package net.minetest.minetest;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.List;

public class FtpActivity extends AppCompatActivity {
	private EditText etPort;
	private EditText etUsername;
	private EditText etPassword;
	private ImageButton btnTogglePassword;
	private TextView tvStatus;
	private TextView tvIpAddress;
	private Button btnToggle;

	private FtpSettingsStore settingsStore;
	private boolean isPasswordVisible = false;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_ftp);

		ActionBar actionBar = getSupportActionBar();
		if (actionBar != null) {
			actionBar.setTitle("FTP Server");
			actionBar.setDisplayHomeAsUpEnabled(true);
		}

		settingsStore = new FtpSettingsStore(this);

		etPort = findViewById(R.id.et_port);
		etUsername = findViewById(R.id.et_username);
		etPassword = findViewById(R.id.et_password);
		btnTogglePassword = findViewById(R.id.btn_toggle_password);
		tvStatus = findViewById(R.id.tv_status);
		tvIpAddress = findViewById(R.id.tv_ip_address);
		btnToggle = findViewById(R.id.btn_toggle);

		etPort.setText(String.valueOf(settingsStore.getPort()));
		etUsername.setText(settingsStore.getUsername());
		etPassword.setText(settingsStore.getPassword());

		btnTogglePassword.setOnClickListener(v -> togglePasswordVisibility());
		btnToggle.setOnClickListener(v -> toggleFtpServer());

		updateUiState();
	}

	private void togglePasswordVisibility() {
		isPasswordVisible = !isPasswordVisible;
		if (isPasswordVisible) {
			etPassword.setTransformationMethod(HideReturnsTransformationMethod.getInstance());
			btnTogglePassword.setImageResource(R.drawable.ic_eye);
		} else {
			etPassword.setTransformationMethod(PasswordTransformationMethod.getInstance());
			btnTogglePassword.setImageResource(R.drawable.ic_eye_off);
		}
		etPassword.setSelection(etPassword.getText().length());
	}

	@Override
	protected void onResume() {
		super.onResume();
		updateUiState();
	}

	private void updateUiState() {
		boolean running = FtpServerManager.getInstance().isRunning();
		if (running) {
			tvStatus.setText("Status: RUNNING");
			btnToggle.setText("Stop FTP Server");
			etPort.setEnabled(false);
			etUsername.setEnabled(false);
			etPassword.setEnabled(false);

			String ip = getLocalIpAddress();
			int port = settingsStore.getPort();
			tvIpAddress.setText("URL: ftp://" + ip + ":" + port + "/");
		} else {
			tvStatus.setText("Status: STOPPED");
			btnToggle.setText("Start FTP Server");
			etPort.setEnabled(true);
			etUsername.setEnabled(true);
			etPassword.setEnabled(true);
			tvIpAddress.setText("URL: N/A");
		}
	}

	private void toggleFtpServer() {
		boolean running = FtpServerManager.getInstance().isRunning();
		if (running) {
			Intent stopIntent = new Intent(this, FtpService.class);
			stopIntent.setAction(FtpService.ACTION_STOP);
			startService(stopIntent);
			Toast.makeText(this, "Stopping FTP Server...", Toast.LENGTH_SHORT).show();
		} else {
			String portStr = etPort.getText().toString().trim();
			String username = etUsername.getText().toString().trim();
			String password = etPassword.getText().toString().trim();

			if (portStr.isEmpty() || username.isEmpty() || password.isEmpty()) {
				Toast.makeText(this, "Please fill in all fields", Toast.LENGTH_SHORT).show();
				return;
			}

			int port = Integer.parseInt(portStr);
			settingsStore.setPort(port);
			settingsStore.setUsername(username);
			settingsStore.setPassword(password);

			Intent startIntent = new Intent(this, FtpService.class);
			startIntent.setAction(FtpService.ACTION_START);
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
				startForegroundService(startIntent);
			} else {
				startService(startIntent);
			}
			Toast.makeText(this, "Starting FTP Server...", Toast.LENGTH_SHORT).show();
		}

		etPort.postDelayed(this::updateUiState, 500);
	}

	private String getLocalIpAddress() {
		try {
			List<NetworkInterface> interfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
			for (NetworkInterface ni : interfaces) {
				List<InetAddress> addrs = Collections.list(ni.getInetAddresses());
				for (InetAddress addr : addrs) {
					if (!addr.isLoopbackAddress()) {
						String sAddr = addr.getHostAddress();
						boolean isIPv4 = sAddr.indexOf(':') < 0;
						if (isIPv4) {
							return sAddr;
						}
					}
				}
			}
		} catch (Exception ignored) {}
		return "127.0.0.1";
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		if (item.getItemId() == android.R.id.home) {
			finish();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
}