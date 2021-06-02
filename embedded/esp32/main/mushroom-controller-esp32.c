/* Blink Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include "../../lib/mushroom.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "twi.h"

#define TAG "bmc"

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
  uint8_t mac_addr[6] = {0};
  /* we can get the ethernet driver handle from event data */
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

  switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
      esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
      ESP_LOGI(TAG, "Ethernet Link Up");
      ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
               mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
               mac_addr[5]);
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "Ethernet Link Down");
      break;
    case ETHERNET_EVENT_START:
      ESP_LOGI(TAG, "Ethernet Started");
      break;
    case ETHERNET_EVENT_STOP:
      ESP_LOGI(TAG, "Ethernet Stopped");
      break;
    default:
      break;
  }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;

  ESP_LOGI(TAG, "Ethernet Got IP Address");
  ESP_LOGI(TAG, "~~~~~~~~~~~");
  ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(TAG, "~~~~~~~~~~~");
}

void app_main(void) {
  // Initialize TCP/IP network interface (should be called only once in
  // application)
  ESP_ERROR_CHECK(esp_netif_init());
  // Create default event loop that running in background
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t *eth_netif = esp_netif_new(&cfg);
  // Set default handlers to process TCP/IP stuffs
  ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));
  // Register user defined event handers
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                             &eth_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                             &got_ip_event_handler, NULL));

  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.phy_addr = -1;
  phy_config.reset_gpio_num = GPIO_NUM_16;
  mac_config.smi_mdc_gpio_num = GPIO_NUM_23;
  mac_config.smi_mdio_gpio_num = GPIO_NUM_18;
  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
  esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);
  esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_handle_t eth_handle = NULL;
  ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
  /* attach Ethernet driver to TCP/IP stack */
  ESP_ERROR_CHECK(
      esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
  /* start Ethernet driver state machine */
  ESP_ERROR_CHECK(esp_eth_start(eth_handle));

  // Configure the TWI client.
  twi_client_configure(1, GPIO_NUM_2, GPIO_NUM_4);

  uint8_t led_state = 0xFF;

  while (1) {
    // Discover all available TWI servers.
    twi_servers_t *servers = twi_client_discover_servers();

    // Log discovered servers.
    ESP_LOGI(TAG, "Detected %d servers:", servers->count);
    for (uint8_t i = 0; i < servers->count; ++i) {
      ESP_LOGI(TAG, "  %X", servers->addresses[i]);
      twi_client_write_byte(servers->addresses[i], REG_LEDMAN, &led_state);
    }

    // Toggle LED state.
    led_state = ~led_state;

    // TODO: Remove this. It is only useful for debugging.
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
