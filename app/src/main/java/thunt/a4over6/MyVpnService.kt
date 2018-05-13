package thunt.a4over6

import android.content.Intent
import android.net.VpnService
import android.os.ParcelFileDescriptor

/**
 * Created by DELL on 2018/5/9.
 */

class MyVpnService: VpnService() {
    lateinit var virtualDescriptor: ParcelFileDescriptor
    var builder = Builder()

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        builder.setMtu(1000)
        builder.addAddress(intent?.getStringExtra("ip"), 24)
        builder.addRoute(intent?.getStringExtra("route"), 0)
        builder.addDnsServer(intent?.getStringExtra("dns1"))
        builder.setSession("MyVpnService")
        virtualDescriptor = builder.establish()
        return super.onStartCommand(intent, flags, startId)
    }
}