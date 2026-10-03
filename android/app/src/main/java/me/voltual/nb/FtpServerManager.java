package me.voltual.nb;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;

import org.apache.ftpserver.ConnectionConfigFactory;
import org.apache.ftpserver.FtpServer;
import org.apache.ftpserver.FtpServerFactory;
import org.apache.ftpserver.ftplet.Authority;
import org.apache.ftpserver.listener.ListenerFactory;
import org.apache.ftpserver.usermanager.impl.BaseUser;
import org.apache.ftpserver.usermanager.impl.WritePermission;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class FtpServerManager {
	private static final String TAG = "FtpServerManager";
	private static FtpServerManager instance;

	private FtpServer ftpServer;
	private boolean isRunning = false;

	private FtpServerManager() {}

	public static synchronized FtpServerManager getInstance() {
		if (instance == null) {
			instance = new FtpServerManager();
		}
		return instance;
	}

	public synchronized boolean isRunning() {
		return isRunning && ftpServer != null && !ftpServer.isStopped();
	}

	public synchronized boolean startServer(@NonNull Context context, int port, @NonNull String username, @NonNull String password) {
		if (isRunning()) {
			Log.i(TAG, "FTP Server is already running.");
			return true;
		}

		try {
			File rootDir = Utils.getUserDataDirectory(context);
			if (!rootDir.exists()) {
				rootDir.mkdirs();
			}

			FtpServerFactory serverFactory = new FtpServerFactory();
			ListenerFactory listenerFactory = new ListenerFactory();
			listenerFactory.setPort(port);

			serverFactory.addListener("default", listenerFactory.createListener());

			BaseUser user = new BaseUser();
			user.setName(username);
			user.setPassword(password);
			user.setHomeDirectory(rootDir.getAbsolutePath());

			List<Authority> authorities = new ArrayList<>();
			authorities.add(new WritePermission());
			user.setAuthorities(authorities);

			serverFactory.getUserManager().save(user);

			ConnectionConfigFactory connectionConfigFactory = new ConnectionConfigFactory();
			connectionConfigFactory.setAnonymousLoginEnabled(false);
			connectionConfigFactory.setMaxLoginFailures(3);
			serverFactory.setConnectionConfig(connectionConfigFactory.createConnectionConfig());

			ftpServer = serverFactory.createServer();
			ftpServer.start();
			isRunning = true;
			Log.i(TAG, "FTP Server started on port " + port + ", root: " + rootDir.getAbsolutePath());
			return true;
		} catch (Exception e) {
			Log.e(TAG, "Failed to start FTP server", e);
			stopServer();
			return false;
		}
	}

	public synchronized void stopServer() {
		if (ftpServer != null) {
			try {
				ftpServer.stop();
				Log.i(TAG, "FTP Server stopped.");
			} catch (Exception e) {
				Log.e(TAG, "Error while stopping FTP server", e);
			}
			ftpServer = null;
		}
		isRunning = false;
	}
}