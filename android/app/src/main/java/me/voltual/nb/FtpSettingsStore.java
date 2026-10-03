package me.voltual.nb;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Base64;
import android.util.Log;

import androidx.annotation.NonNull;

import com.google.crypto.tink.Aead;
import com.google.crypto.tink.KeyTemplate;
import com.google.crypto.tink.RegistryConfiguration;
import com.google.crypto.tink.aead.AeadConfig;
import com.google.crypto.tink.aead.PredefinedAeadParameters;
import com.google.crypto.tink.integration.android.AndroidKeysetManager;

import java.nio.charset.StandardCharsets;
import java.util.UUID;

public class FtpSettingsStore {
	private static final String TAG = "FtpSettingsStore";
	private static final String PREF_NAME = "ftp_settings_prefs";
	private static final String KEY_PORT = "ftp_port";
	private static final String KEY_USERNAME = "ftp_username";
	private static final String KEY_PASSWORD_ENC = "ftp_password_enc";

	private static final String KEYSET_NAME = "ftp_keyset";
	private static final String KEYSET_PREF_NAME = "ftp_keyset_prefs";
	private static final String MASTER_KEY_URI = "android-keystore://ftp_master_key";
	private static final byte[] ASSOCIATED_DATA = "ftp_auth_data".getBytes(StandardCharsets.UTF_8);

	private final SharedPreferences preferences;
	private Aead aead;

	public FtpSettingsStore(@NonNull Context context) {
		preferences = context.getApplicationContext().getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
		initTink(context.getApplicationContext());
	}

	private void initTink(Context context) {
		try {
			AeadConfig.register();
			AndroidKeysetManager keysetManager = new AndroidKeysetManager.Builder()
				.withSharedPref(context, KEYSET_NAME, KEYSET_PREF_NAME)
				.withKeyTemplate(KeyTemplate.createFrom(PredefinedAeadParameters.AES256_GCM))
				.withMasterKeyUri(MASTER_KEY_URI)
				.build();
			aead = keysetManager.getKeysetHandle().getPrimitive(RegistryConfiguration.get(), Aead.class);
		} catch (Exception e) {
			Log.e(TAG, "Failed to initialize Tink AEAD", e);
		}
	}

	public int getPort() {
		return preferences.getInt(KEY_PORT, 2121);
	}

	public void setPort(int port) {
		preferences.edit().putInt(KEY_PORT, port).apply();
	}

	@NonNull
	public String getUsername() {
		return preferences.getString(KEY_USERNAME, "admin");
	}

	public void setUsername(@NonNull String username) {
		preferences.edit().putString(KEY_USERNAME, username).apply();
	}

	@NonNull
	public String getPassword() {
		String encPass = preferences.getString(KEY_PASSWORD_ENC, null);
		if (encPass == null || encPass.isEmpty()) {
			String defaultPass = UUID.randomUUID().toString().substring(0, 6);
			setPassword(defaultPass);
			return defaultPass;
		}

		if (aead == null) {
			return "admin";
		}

		try {
			byte[] cipherText = Base64.decode(encPass, Base64.DEFAULT);
			byte[] decrypted = aead.decrypt(cipherText, ASSOCIATED_DATA);
			return new String(decrypted, StandardCharsets.UTF_8);
		} catch (Exception e) {
			Log.e(TAG, "Failed to decrypt password", e);
			return "admin";
		}
	}

	public void setPassword(@NonNull String password) {
		if (aead == null) {
			preferences.edit().putString(KEY_PASSWORD_ENC, password).apply();
			return;
		}

		try {
			byte[] cipherText = aead.encrypt(password.getBytes(StandardCharsets.UTF_8), ASSOCIATED_DATA);
			String base64Enc = Base64.encodeToString(cipherText, Base64.NO_WRAP);
			preferences.edit().putString(KEY_PASSWORD_ENC, base64Enc).apply();
		} catch (Exception e) {
			Log.e(TAG, "Failed to encrypt password", e);
		}
	}
}