package me.voltual.nb;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

public class FtpService extends Service {
	private static final String TAG = "FtpService";
	public static final String ACTION_START = "me.voltual.nb.FTP_START";
	public static final String ACTION_STOP = "me.voltual.nb.FTP_STOP";
	public static final int NOTIFICATION_ID_FTP = 1001;

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		String action = intent != null ? intent.getAction() : null;
		if (ACTION_STOP.equals(action)) {
			stopForeground(true);
			stopSelf();
			return START_NOT_STICKY;
		}

		FtpSettingsStore settingsStore = new FtpSettingsStore(this);
		int port = settingsStore.getPort();
		String username = settingsStore.getUsername();
		String password = settingsStore.getPassword();

		boolean success = FtpServerManager.getInstance().startServer(this, port, username, password);
		if (success) {
			startForeground(NOTIFICATION_ID_FTP, createNotification());
		} else {
			Log.e(TAG, "Failed to start FTP server, stopping service.");
			stopSelf();
		}

		return START_STICKY;
	}

	private Notification createNotification() {
		NotificationManager notifyManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			NotificationChannel channel = new NotificationChannel(
				MainActivity.NOTIFICATION_CHANNEL_ID,
				getString(R.string.notification_channel_name),
				NotificationManager.IMPORTANCE_LOW
			);
			channel.setSound(null, null);
			channel.enableLights(false);
			channel.enableVibration(false);
			if (notifyManager != null) {
				notifyManager.createNotificationChannel(channel);
			}
		}

		Intent notificationIntent = new Intent(this, FtpActivity.class);
		notificationIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
		
		int pendingIntentFlag = 0;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
			pendingIntentFlag = PendingIntent.FLAG_MUTABLE;
		}
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, pendingIntentFlag);

		NotificationCompat.Builder builder;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			builder = new NotificationCompat.Builder(this, MainActivity.NOTIFICATION_CHANNEL_ID);
		} else {
			builder = new NotificationCompat.Builder(this);
		}

		builder.setContentTitle("FTP server")
			.setContentText("FTP server is running")
			.setSmallIcon(R.mipmap.ic_launcher)
			.setContentIntent(contentIntent)
			.setOngoing(true);

		return builder.build();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		FtpServerManager.getInstance().stopServer();
		Log.i(TAG, "FtpService destroyed");
	}

	@Nullable
	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
}