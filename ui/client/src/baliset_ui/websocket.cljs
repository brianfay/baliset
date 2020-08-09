(ns baliset-ui.websocket
  (:require [re-frame.core :as rf]
            [goog.object :as goo]))

(def port "3002")

(defonce sock (atom nil))

(defn handle-message [e]
  (let [msg (.parse js/JSON (goo/get e "data"))]
    (case (goo/get msg "route")
      "/app_state"
      (rf/dispatch [:app-state msg])
      "/node/added"
      (rf/dispatch [:db-add-node (goo/get msg "node_id") (goo/get msg "type") (goo/get msg "x") (goo/get msg "y")])
      "/node/deleted"
      (rf/dispatch [:db-delete-node (goo/get msg "node_id")])
      "/node/connected"
      (rf/dispatch [:db-connect-node (goo/get msg "out_node_id") (goo/get msg "outlet_idx")
                    (goo/get msg "in_node_id") (goo/get msg "inlet_idx")])
      "/node/disconnected"
      (rf/dispatch [:db-disconnect-node (goo/get msg "out_node_id")  (goo/get msg "outlet_idx")
                    (goo/get msg "in_node_id") (goo/get msg "inlet_idx")])
      "/node/moved"
      (rf/dispatch [:move-node (goo/get msg "node_id") (goo/get msg "x") (goo/get msg "y")])

      "/node/controlled"
      (rf/dispatch [:set-control (goo/get msg "node_id") (goo/get msg "control_id") (goo/get msg "value")])

      "/patch/saved"
      (rf/dispatch [:patch-saved (goo/get msg "name")])

      ;;TODO do something on /patch/saved
      (println "unrecognized msg: " msg))))

(declare handle-closed-socket)

(defn connect []
  (when-not @sock
    (let [ws (js/WebSocket. (str "ws://" (goo/getValueByKeys js/window "location" "hostname") ":" port))]
      (.addEventListener ws "open" #(rf/dispatch [:set-server-error-msg nil]))
      (.addEventListener ws "message" handle-message)
      (.addEventListener ws "close" handle-closed-socket)
      (.addEventListener ws "error" handle-closed-socket)
      (reset! sock ws))))

(defn handle-closed-socket [e]
  (rf/dispatch [:set-server-error-msg "lost websocket connection :( ... attempting to reconnect"])
  (reset! sock nil)
  (js/setTimeout connect 1000))

(defn close []
  (.close @sock)
  (reset! sock nil))

(rf/reg-fx
 :ws-add-node
 (fn [[node-type x y]]
   (.send @sock (.stringify js/JSON #js {:route "/node/add" :type node-type :x x :y y}))))

(rf/reg-fx
 :ws-delete-node
 (fn [[id]]
   (.send @sock (.stringify js/JSON #js {:route "/node/delete" :node_id id}))))

(rf/reg-fx
 :ws-move-node
 (fn [[id x y]]
   (.send @sock (.stringify js/JSON #js {:route "/node/move" :node_id id :x x :y y}))))

(rf/reg-fx
 :ws-control-node
 (fn [[node-id ctl-id val]]
   (.send @sock (.stringify js/JSON #js {:route "/node/control" :node_id node-id :control_id ctl-id :value val}))))

(rf/reg-fx
 :ws-connect
 (fn [[out-node-id outlet-idx in-node-id inlet-idx]]
   (.send @sock (.stringify js/JSON #js {:route "/node/connect"
                                         :out_node_id out-node-id
                                         :outlet_idx outlet-idx
                                         :in_node_id in-node-id
                                         :inlet_idx inlet-idx}))))

(rf/reg-fx
 :ws-disconnect
 (fn [[out-node-id outlet-idx in-node-id inlet-idx]]
   (.send @sock (.stringify js/JSON #js {:route "/node/disconnect"
                                         :out_node_id out-node-id
                                         :outlet_idx outlet-idx
                                         :in_node_id in-node-id
                                         :inlet_idx inlet-idx}))))

(rf/reg-fx
 :ws-load-patch
 (fn [[patch-name]]
   (.send @sock (.stringify js/JSON #js {:route "/patch/load" :name patch-name}))))

(rf/reg-fx
 :ws-save-patch
 (fn [[patch-name]]
   (.send @sock (.stringify js/JSON #js {:route "/patch/save" :name patch-name}))))

(comment
  (close)
  (connect)
  (rf/dispatch [:clicked-add-node "dac" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "sin" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "adc" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "delay" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "looper" (rand-int 500) (rand-int 500)])
  (.send @sock (.stringify js/JSON #js {:route "/patch/save" :name "patchy"}))
  (.send @sock (.stringify js/JSON #js {:route "/patch/load" :name "patchy"}))
  (.send @sock (.stringify js/JSON #js {:route "/patch/save" :name "sine"}))
  (.send @sock (.stringify js/JSON #js {:route "/patch/load" :name "sine"}))
  )
