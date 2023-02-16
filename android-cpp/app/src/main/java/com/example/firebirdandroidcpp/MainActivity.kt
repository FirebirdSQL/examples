package com.example.firebirdandroidcpp

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.example.firebirdandroidcpp.databinding.ActivityMainBinding
import org.firebirdsql.android.embedded.FirebirdConf
import java.io.File

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        FirebirdConf.extractAssets(baseContext, false)
        FirebirdConf.setEnv(baseContext)

        connect(File(filesDir, "test.fdb").absolutePath)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        try {
            binding.sampleText.text = getCurrentTimestamp()
        }
        catch (e: Exception) {
            binding.sampleText.text = "Error: ${e.message}"
        }
    }

    override fun onDestroy() {
        disconnect();
        super.onDestroy()
    }

    private external fun connect(databaseName: String)
    private external fun disconnect()
    private external fun getCurrentTimestamp(): String

    companion object {
        // Used to load the 'firebirdandroidcpp' library on application startup.
        init {
            System.loadLibrary("firebirdandroidcpp")
        }
    }
}
