package com.mixpractice.studio;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.billingclient.api.AcknowledgePurchaseParams;
import com.android.billingclient.api.AcknowledgePurchaseResponseListener;
import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.PendingPurchasesParams;
import com.android.billingclient.api.ProductDetails;
import com.android.billingclient.api.ProductDetailsResponseListener;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesResponseListener;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.QueryProductDetailsParams;
import com.android.billingclient.api.QueryPurchasesParams;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Handles Google Play In-App Billing for Mix Practice Studio.
 * Supports Lifetime One-Time Purchase (mps_pro_lifetime), purchase acknowledgment,
 * and purchase restoration across device reinstallations.
 */
public class PlayBillingHelper implements PurchasesUpdatedListener {
    private static final String TAG = "PlayBillingHelper";

    public static final String PRODUCT_ID_PRO_LIFETIME = "mps_pro_lifetime";
    private static final String PREF_NAME = "mps_play_billing";
    private static final String KEY_IS_LIFETIME_PRO = "is_lifetime_pro";
    private static final String KEY_PURCHASE_TOKEN = "pro_purchase_token";

    private static volatile PlayBillingHelper instance;

    private final Context appContext;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private BillingClient billingClient;
    private ProductDetails proProductDetails;
    private boolean isConnecting = false;

    // Interface for C++/JNI and UI callbacks
    public interface BillingCallback {
        void onProStatusChanged(boolean isProLifetime, String message);
    }

    private static BillingCallback billingCallback;

    public static void setBillingCallback(BillingCallback callback) {
        billingCallback = callback;
    }

    public static synchronized PlayBillingHelper getInstance(Context context) {
        if (instance == null) {
            instance = new PlayBillingHelper(context.getApplicationContext());
        }
        return instance;
    }

    private PlayBillingHelper(Context context) {
        this.appContext = context;
        initBillingClient();
    }

    private void initBillingClient() {
        PendingPurchasesParams pendingParams = PendingPurchasesParams.newBuilder()
                .enableOneTimeProducts()
                .build();

        billingClient = BillingClient.newBuilder(appContext)
                .setListener(this)
                .enablePendingPurchases(pendingParams)
                .build();

        startConnection(new Runnable() {
            @Override
            public void run() {
                queryProProductDetails();
                queryExistingPurchases(null);
            }
        });
    }

    public void startConnection(final Runnable onConnectedAction) {
        if (billingClient == null) return;
        if (billingClient.isReady()) {
            if (onConnectedAction != null) {
                onConnectedAction.run();
            }
            return;
        }

        if (isConnecting) return;
        isConnecting = true;

        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                isConnecting = false;
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    Log.d(TAG, "Billing service connected successfully.");
                    if (onConnectedAction != null) {
                        onConnectedAction.run();
                    }
                } else {
                    Log.w(TAG, "Billing setup failed: " + billingResult.getDebugMessage());
                }
            }

            @Override
            public void onBillingServiceDisconnected() {
                isConnecting = false;
                Log.w(TAG, "Billing service disconnected.");
            }
        });
    }

    public void queryProProductDetails() {
        if (billingClient == null || !billingClient.isReady()) {
            startConnection(new Runnable() {
                @Override
                public void run() {
                    queryProProductDetails();
                }
            });
            return;
        }

        List<QueryProductDetailsParams.Product> productList = new ArrayList<>();
        productList.add(
                QueryProductDetailsParams.Product.newBuilder()
                        .setProductId(PRODUCT_ID_PRO_LIFETIME)
                        .setProductType(BillingClient.ProductType.INAPP)
                        .build()
        );

        QueryProductDetailsParams params = QueryProductDetailsParams.newBuilder()
                .setProductList(productList)
                .build();

        billingClient.queryProductDetailsAsync(params, new ProductDetailsResponseListener() {
            @Override
            public void onProductDetailsResponse(@NonNull BillingResult billingResult,
                                                 @NonNull List<ProductDetails> list) {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK && !list.isEmpty()) {
                    proProductDetails = list.get(0);
                    Log.d(TAG, "Fetched product: " + proProductDetails.getName() + " - " + proProductDetails.getProductId());
                } else {
                    Log.w(TAG, "Failed to query product details: " + billingResult.getDebugMessage());
                }
            }
        });
    }

    /**
     * Initiates Google Play purchase flow for Lifetime Pro.
     */
    public void launchPurchaseFlow(final Activity activity) {
        if (activity == null) return;

        if (billingClient == null || !billingClient.isReady()) {
            startConnection(new Runnable() {
                @Override
                public void run() {
                    launchPurchaseFlow(activity);
                }
            });
            return;
        }

        if (proProductDetails == null) {
            // Re-fetch product details and launch
            List<QueryProductDetailsParams.Product> productList = new ArrayList<>();
            productList.add(
                    QueryProductDetailsParams.Product.newBuilder()
                            .setProductId(PRODUCT_ID_PRO_LIFETIME)
                            .setProductType(BillingClient.ProductType.INAPP)
                            .build()
            );

            QueryProductDetailsParams params = QueryProductDetailsParams.newBuilder()
                    .setProductList(productList)
                    .build();

            billingClient.queryProductDetailsAsync(params, new ProductDetailsResponseListener() {
                @Override
                public void onProductDetailsResponse(@NonNull BillingResult billingResult,
                                                     @NonNull List<ProductDetails> list) {
                    if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK && !list.isEmpty()) {
                        proProductDetails = list.get(0);
                        mainHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                executeBillingFlow(activity, proProductDetails);
                            }
                        });
                    } else {
                        notifyCallback(false, "ไม่พบสินค้าใน Google Play กรุณาลองใหม่อีกครั้ง");
                    }
                }
            });
            return;
        }

        executeBillingFlow(activity, proProductDetails);
    }

    private void executeBillingFlow(Activity activity, ProductDetails productDetails) {
        List<BillingFlowParams.ProductDetailsParams> detailsParamsList = Collections.singletonList(
                BillingFlowParams.ProductDetailsParams.newBuilder()
                        .setProductDetails(productDetails)
                        .build()
        );

        BillingFlowParams billingFlowParams = BillingFlowParams.newBuilder()
                .setProductDetailsParamsList(detailsParamsList)
                .build();

        BillingResult result = billingClient.launchBillingFlow(activity, billingFlowParams);
        if (result.getResponseCode() != BillingClient.BillingResponseCode.OK) {
            Log.e(TAG, "launchBillingFlow failed: " + result.getDebugMessage());
            notifyCallback(false, "ไม่สามารถเปิดหน้าชำระเงิน Google Play ได้ (" + result.getDebugMessage() + ")");
        }
    }

    /**
     * Restore Purchases: Queries Google Play for active purchases previously made with user's Google account.
     */
    public void restorePurchases(final Runnable onComplete) {
        startConnection(new Runnable() {
            @Override
            public void run() {
                queryExistingPurchases(onComplete);
            }
        });
    }

    private void queryExistingPurchases(final Runnable onComplete) {
        QueryPurchasesParams queryPurchasesParams = QueryPurchasesParams.newBuilder()
                .setProductType(BillingClient.ProductType.INAPP)
                .build();

        billingClient.queryPurchasesAsync(queryPurchasesParams, new PurchasesResponseListener() {
            @Override
            public void onQueryPurchasesResponse(@NonNull BillingResult billingResult,
                                                 @NonNull List<Purchase> list) {
                boolean foundPro = false;
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    for (Purchase purchase : list) {
                        if (purchase.getProducts().contains(PRODUCT_ID_PRO_LIFETIME)) {
                            if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                                foundPro = true;
                                handlePurchase(purchase, true);
                            }
                        }
                    }
                }

                if (!foundPro && isLocallyUnlocked()) {
                    // Purchase was previously verified
                    foundPro = true;
                }

                final boolean finalFound = foundPro;
                mainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (onComplete != null) {
                            onComplete.run();
                        }
                        if (finalFound) {
                            notifyCallback(true, "กู้คืนสิทธิ์ Pro Lifetime สำเร็จ!");
                        } else {
                            notifyCallback(false, "ไม่พบประวัติการซื้อ Pro ด้วยบัญชี Google นี้");
                        }
                    }
                });
            }
        });
    }

    @Override
    public void onPurchasesUpdated(@NonNull BillingResult billingResult, @Nullable List<Purchase> purchases) {
        int responseCode = billingResult.getResponseCode();
        if (responseCode == BillingClient.BillingResponseCode.OK && purchases != null) {
            for (Purchase purchase : purchases) {
                handlePurchase(purchase, false);
            }
        } else if (responseCode == BillingClient.BillingResponseCode.USER_CANCELED) {
            Log.d(TAG, "User canceled the purchase flow.");
            notifyCallback(false, "การซื้อถูกยกเลิก");
        } else {
            Log.w(TAG, "Purchase failed: " + billingResult.getDebugMessage());
            notifyCallback(false, "การสั่งซื้อไม่สำเร็จ: " + billingResult.getDebugMessage());
        }
    }

    private void handlePurchase(final Purchase purchase, final boolean isRestore) {
        if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
            // Acknowledge the purchase if not already acknowledged
            if (!purchase.isAcknowledged()) {
                AcknowledgePurchaseParams acknowledgePurchaseParams =
                        AcknowledgePurchaseParams.newBuilder()
                                .setPurchaseToken(purchase.getPurchaseToken())
                                .build();

                billingClient.acknowledgePurchase(acknowledgePurchaseParams, new AcknowledgePurchaseResponseListener() {
                    @Override
                    public void onAcknowledgePurchaseResponse(@NonNull BillingResult billingResult) {
                        if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                            Log.d(TAG, "Purchase acknowledged successfully: " + purchase.getOrderId());
                            saveProUnlocked(purchase.getPurchaseToken());
                            notifyCallback(true, isRestore ? "กู้คืนสิทธิ์ Pro Lifetime สำเร็จ!" : "ยินดีด้วย! คุณได้รับสิทธิ์ Pro Lifetime ถาวรเรียบร้อยแล้ว");
                        } else {
                            Log.e(TAG, "Failed to acknowledge purchase: " + billingResult.getDebugMessage());
                        }
                    }
                });
            } else {
                saveProUnlocked(purchase.getPurchaseToken());
                notifyCallback(true, isRestore ? "กู้คืนสิทธิ์ Pro Lifetime สำเร็จ!" : "คุณมีสิทธิ์ Pro Lifetime อยู่แล้ว");
            }
        }
    }

    private void saveProUnlocked(String purchaseToken) {
        SharedPreferences sp = appContext.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        sp.edit()
                .putBoolean(KEY_IS_LIFETIME_PRO, true)
                .putString(KEY_PURCHASE_TOKEN, purchaseToken)
                .apply();

        // Also inform C++ AppEdition to write/update pro_license_v2.json with plan=lifetime
        notifyNativeProActivated();
    }

    public boolean isLocallyUnlocked() {
        SharedPreferences sp = appContext.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        return sp.getBoolean(KEY_IS_LIFETIME_PRO, false);
    }

    private void notifyCallback(final boolean isPro, final String message) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (billingCallback != null) {
                    billingCallback.onProStatusChanged(isPro, message);
                }
            }
        });
    }

    // JNI bridge methods
    public static void buyProLifetime(Activity activity) {
        if (activity == null) return;
        getInstance(activity).launchPurchaseFlow(activity);
    }

    public static void restoreProPurchases(Activity activity) {
        if (activity == null) return;
        getInstance(activity).restorePurchases(null);
    }

    public static boolean checkIsProLifetime(Context context) {
        if (context == null) return false;
        return getInstance(context).isLocallyUnlocked();
    }

    // Native C++ notification
    public static native void nativeOnProUnlocked(boolean isLifetime, String token);

    private void notifyNativeProActivated() {
        try {
            nativeOnProUnlocked(true, "google_play_verified");
        } catch (UnsatisfiedLinkError e) {
            Log.d(TAG, "nativeOnProUnlocked not bound yet: " + e.getMessage());
        }
    }
}
