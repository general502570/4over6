package thunt.a4over6

import android.content.Intent
import android.net.VpnService
import android.os.ParcelFileDescriptor
import java.io.BufferedWriter
import java.io.File

/**
 * Created by DELL on 2018/5/9.
 */

class MyVpnService: VpnService() {
    lateinit var virtualDescriptor: ParcelFileDescriptor
    var builder = Builder()
    val JNI_DES_PIPE_PATH   = "/data/data/thunt.a4over6/4over6.des"

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        println("on VPN start")
        builder.setMtu(1000)
        builder.addAddress(intent?.getStringExtra("ip"), 24)
        builder.addRoute(intent?.getStringExtra("route"), 0)
        builder.addDnsServer(intent?.getStringExtra("dns1"))
        builder.setSession("MyVpnService")
        virtualDescriptor = builder.establish()
        WritePipe()
        return super.onStartCommand(intent, flags, startId)
    }

    fun WritePipe() {
        val cont = virtualDescriptor.fd
        println("VPN started")
        println(cont)
        val file = File(JNI_DES_PIPE_PATH)
        val bufferWriter: BufferedWriter = file.bufferedWriter()
        bufferWriter.use { out->out.write(cont) }
        bufferWriter.close()
    }
}