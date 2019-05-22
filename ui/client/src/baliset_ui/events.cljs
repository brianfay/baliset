(ns baliset-ui.events
  (:require [re-frame.core :as rf]
            [ajax.core :as ajax]
            [clojure.set :as cset]))

(defn local-route [route]
  ;; (str (.-location js/window) route)
  (str "http://192.168.0.104:3001/" route)
  ;; (str "http://localhost:3001/" route)
  )

(rf/reg-event-db
 :initialize-db
 (fn [db _]
   (assoc db :node-metadata {}
             :connections #{}
             :nodes {}
             :node-offset {}
             :pan [0 0]
             :pan-offset [0 0]
             :inlet-offset {}
             :outlet-offset {}
             :selected-io nil
             :selected-node nil
             :left-nav-expanded? false)))

(defonce db-init (rf/dispatch-sync [:initialize-db]))

(rf/reg-event-db
 :load-node-metadata
 (fn [db [_ result]]
   (let [node-map (reduce (fn [acc node-data]
                            (assoc acc (node-data "name") node-data))
                          {}
                          result)]
     (assoc db :node-metadata node-map))))

(rf/reg-event-db
 :app-state
 (fn [db [_ resp]]
   (-> db
       (assoc :nodes
              (reduce-kv
               (fn [m k v] (assoc m (int (name k)) v))
               {}
               (js->clj (.-nodes resp) :keywordize-keys true)))
       (update :connections into (js->clj (.-connections resp))))))

(rf/reg-event-db
 :node-metadata-failure ;;TODO
 (fn [db [_ result]]
   db))

(rf/reg-event-db
 :reg-inlet-offset
 (fn [db [_ node-id idx offset-top offset-height]]
   (assoc-in db [:inlet-offset node-id idx] [offset-top offset-height])))

(rf/reg-event-db
 :unreg-inlet-offset
 (fn [db [_ node-id idx]]
   (update-in db [:inlet-offset node-id] dissoc idx)))

(rf/reg-event-db
 :reg-outlet-offset
 (fn [db [_ node-id idx offset-top offset-height offset-left offset-width]]
   (assoc-in db [:outlet-offset node-id idx] [offset-top offset-height offset-left offset-width])))

(rf/reg-event-db
 :unreg-outlet-offset
 (fn [db [_ node-id idx]]
   (update-in db [:outlet-offset node-id] dissoc idx)))

(rf/reg-event-db
 :clicked-add-btn
 (fn [db [_ node-name]]
   (if (= (:selected-add-btn db) node-name)
     (assoc db :selected-add-btn nil)
     (assoc db :selected-add-btn node-name))))

(rf/reg-event-db
 :clicked-minimized-left-nav
 (fn [db _]
   (assoc db :left-nav-expanded? true)))

(rf/reg-event-db
 :clicked-expanded-left-nav
 (fn [db _]
   (assoc db :left-nav-expanded? false)))

;;cases
;;nothing selected
;; select inlet
;;inlet already selected
;; unselect
;;other inlet selected
;; select inlet
;;outlet already selected
;;  node id is same
;;    select inlet
;;  node id is different
;;    connect (but ideally check for cycles, maybe indicate with colors)

(rf/reg-event-fx
 :clicked-inlet
 (fn [{:keys [db]} [_ node-id inlet-idx]]
   (let [selected-io (:selected-io db)
         [in-or-out out-node-id outlet-idx] selected-io
         connecting? (and (= in-or-out :outlet) (not= node-id out-node-id))
         already-connected? (and connecting?
                                 (contains? (:connections db)
                                            [out-node-id outlet-idx node-id inlet-idx]))]
     (merge
      {:db (cond
             (= selected-io [:inlet node-id inlet-idx])
             (assoc db :selected-io nil)

             (or already-connected? connecting?)
             db

             :default
             (assoc db :selected-io [:inlet node-id inlet-idx]))}
      (cond already-connected?
            {:ws-disconnect [out-node-id outlet-idx node-id inlet-idx]}

            connecting?
            {:ws-connect [out-node-id outlet-idx node-id inlet-idx]})))))

(rf/reg-event-fx
 :clicked-outlet
 (fn [{:keys [db]} [_ node-id outlet-idx]]
   (let [selected-io (:selected-io db)
         [in-or-out in-node-id inlet-idx] selected-io
         connecting? (and (= in-or-out :inlet) (not= node-id in-node-id))
         already-connected? (and connecting?
                                 (contains? (:connections db)
                                            [node-id outlet-idx in-node-id inlet-idx]))]
     (merge {:db (cond
                   (= selected-io [:outlet node-id outlet-idx])
                   (assoc db :selected-io nil)

                   (or already-connected? connecting?)
                   db

                   :default
                   (assoc db :selected-io [:outlet node-id outlet-idx]))}
            (cond already-connected?
                  {:ws-disconnect [node-id outlet-idx in-node-id inlet-idx]}

                  connecting?
                  {:ws-connect [node-id outlet-idx in-node-id inlet-idx]})))))

(rf/reg-event-fx
 :clicked-app-div
 (fn [{:keys [db]} [_ x y]]
   (let [[pan-x pan-y] (mapv + (:pan db) (:pan-offset db))]
     (merge {}
            (cond (:selected-add-btn db)
                  {:ws-add-node [(:selected-add-btn db) (- x pan-x) (- y pan-y)]}

                  :default
                  {:db (assoc db
                              :selected-io nil
                              :selected-node nil)})))))


(rf/reg-event-db
 :db-add-node
 (fn [db [_ node-id node-type x y]]
   (assoc-in db [:nodes node-id] {:type node-type :x x :y y})))

(rf/reg-event-db
 :db-delete-node
 (fn [db [_ node-id]]
   (let [new-connections (set (filter (fn [conn] (and (not= node-id (nth conn 0))
                                                      (not= node-id (nth conn 2))))
                                      (:connections db)))]
     (-> db
         (update :nodes dissoc node-id)
         (assoc :connections new-connections)))))

(rf/reg-event-db
 :db-connect-node
 (fn [db [_ out-node-id outlet-idx in-node-id inlet-idx]]
   (update db :connections cset/union #{[out-node-id outlet-idx in-node-id inlet-idx]})))

(rf/reg-event-db
 :db-disconnect-node
 (fn [db [_ out-node-id outlet-idx in-node-id inlet-idx]]
   (update db :connections disj [out-node-id outlet-idx in-node-id inlet-idx])))

(rf/reg-event-fx
 :request-node-metadata
 (fn [_ _]
   {:http-xhrio {:method :get
                 :uri (local-route "node_metadata")
                 :response-format (ajax/json-response-format {:keywords? false})
                 :on-failure [:node-metadata-failure]
                 :on-success [:load-node-metadata]}}))

;;TODO velocity based panning?

(rf/reg-event-db
 :drag-node
 (fn [db [_ id x y]]
   (-> db
       (assoc :recently-interacted-node id)
       (assoc-in [:node-offset id] [x y]))))

(rf/reg-event-db
 :clicked-node-header
 (fn [db [_ id]]
   (assoc db :selected-node id)))

(rf/reg-event-fx
 :clicked-delete-node-btn
 (fn [{:keys [db]} _]
   {:ws-delete-node [(:selected-node db)]
    :db (assoc db :selected-node nil)}))

(rf/reg-event-fx
 :finish-drag-node
 (fn [{:keys [db]} [_ id]]
   (let [[x y] (get (:node-offset db) id)
         new-x (+ x (get-in db [:nodes id :x]))
         new-y (+ y (get-in db [:nodes id :y]))]
     {:db (-> db
              (assoc-in [:node-offset id] [0 0])
              (assoc-in [:nodes id :x] new-x)
              (assoc-in [:nodes id :y] new-y))
      :ws-move-node [id new-x new-y]})))

(rf/reg-event-db
 :move-node
 (fn [db [_ id x y]]
   (-> db
       (assoc-in [:nodes id :x] x)
       (assoc-in [:nodes id :y] y))))

(rf/reg-event-db
 :pan-camera
 (fn [db [_ x y]]
   (assoc db :pan-offset [x y])))

(rf/reg-event-db
 :finish-pan-camera
 (fn [db _]
   (assoc db
          :pan (mapv + (:pan db) (:pan-offset db))
          :pan-offset [0 0])))
