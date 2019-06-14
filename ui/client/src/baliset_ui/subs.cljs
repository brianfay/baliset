(ns baliset-ui.subs
  (:require [re-frame.core :as rf]
            [goog.object :as goo]))


(rf/reg-sub
 :all-node-metadata
 (fn [db _]
   (:node-metadata db)))

(rf/reg-sub
 :node-types
 (fn [_ _]
   (rf/subscribe [:all-node-metadata]))
 (fn [node-metadata _]
   (map str (keys node-metadata))))

rf/subscribe
(rf/reg-sub
 :node-metadata
 (fn [_ _]
   (rf/subscribe [:all-node-metadata]))
 (fn [node-metadata [_ node-type]]
   (get node-metadata node-type)))

(rf/reg-sub
 :node-ids
 (fn [db _]
   (keys (:nodes db))))

(rf/reg-sub
 :node
 (fn [db [_ id]]
   (get (:nodes db) id)))

(rf/reg-sub
 :selected-node
 (fn [db _]
   (when-let [selected-node (:selected-node db)]
     (assoc (get (:nodes db) selected-node)
            :id selected-node))))

(rf/reg-sub
 :node-offset
 (fn [db [_ id]]
   (or (get (:node-offset db) id)
       [0 0])))

(rf/reg-sub
 :recently-interacted-node?
 (fn [db [_ node-id]]
   (= (:recently-interacted-node db) node-id)))

(rf/reg-sub
 :app-panel-expanded?
 (fn [db _]
   (:app-panel-expanded? db)))

(rf/reg-sub
 :node-panel-expanded?
 (fn [db _]
   (:node-panel-expanded? db)))

(rf/reg-sub
 :selected-io?
 (fn [db [_ inlet-or-outlet node-id inlet-idx]]
   (= (:selected-io db)
      [inlet-or-outlet node-id inlet-idx])))

(rf/reg-sub
 :selected-add-btn?
 (fn [db [_ node-name]]
   (= (:selected-add-btn db)
      node-name)))

(rf/reg-sub
 :node-position
 (fn [[_ node-id] _]
   [(rf/subscribe [:node node-id])
    (rf/subscribe [:node-offset node-id])])
 (fn [[node node-offset] _]
   (let [{:keys [x y]} node
         [drag-x drag-y] node-offset]
     [(+ x drag-x) (+ y drag-y)])))

(rf/reg-sub
 :connections
 (fn [db _]
   (:connections db)))

(rf/reg-sub
 :outlet-offset
 (fn [db [_ node-id idx]]
   (get-in db [:outlet-offset node-id idx])))

(rf/reg-sub
 :inlet-offset
 (fn [db [_ node-id idx]]
   (get-in db [:inlet-offset node-id idx])))

(rf/reg-sub
 :connection
 (fn [[_ [out-node-id outlet-idx in-node-id inlet-idx]] _]
   [(rf/subscribe [:node-position out-node-id])
    (rf/subscribe [:node-position in-node-id])
    (rf/subscribe [:outlet-offset out-node-id outlet-idx])
    (rf/subscribe [:inlet-offset in-node-id inlet-idx])])
 (fn [[out-pos in-pos out-offset in-offset] _]
   (let [[out-node-x out-node-y] out-pos
         [in-node-x in-node-y] in-pos
         [out-offset-top out-offset-height out-offset-left out-offset-width] out-offset
         [in-offset-top in-offset-height] in-offset
         out-x (+ out-node-x out-offset-left out-offset-width)
         out-y (+ out-node-y out-offset-top (* out-offset-height 0.5))
         in-x in-node-x
         in-y (+ in-node-y in-offset-top (* in-offset-height 0.5))
         cx1 (+ in-x (/ (- out-x in-x) 2))
         cy1 in-y
         cx2 cx1
         cy2 out-y]
     [out-x out-y cx1 cy1 cx2 cy2 in-x in-y])))

;; (rf/reg-sub
;;  :control-value
;;  (fn [db [_ node-id ctl-id]]
;;    (get-in db [:control-value node-id ctl-id])))

(rf/reg-sub
 :ctl-meta
 (fn [db [_ node-type ctl-id]]
   (nth (get-in db [:node-metadata node-type "controls"]) ctl-id)))

(rf/reg-sub
 :hslider-value
 (fn [db [_ node-type node-id ctl-id]]
   (let [ctl-metadata (nth (get-in db [:node-metadata node-type "controls"]) ctl-id)
         [min max] (or (get ctl-metadata "range") [0.0 1.0])
         percent (or (get-in db [:hslider-percent node-id ctl-id])
                     (/ (- (get ctl-metadata "default") min) (- max min))
                     0.0)
         percent-offset (or (get-in db [:hslider-percent-offset node-id ctl-id]) 0.0)]
     (+ min (* (- max min) (+ percent percent-offset))))))

(rf/reg-sub
 :pan
 (fn [db _]
   (:pan db)))

(rf/reg-sub
 :pan-offset
 (fn [db _]
   (:pan-offset db)))

(rf/reg-sub
 :pan-pos
 (fn [_ _]
   [(rf/subscribe [:pan])
    (rf/subscribe [:pan-offset])])
 (fn [[pan pan-offset] _]
   (mapv + pan pan-offset)))

(rf/reg-sub
 :scale
 (fn [db _]
   (+ (- (:pinch-scale db) 1)
      (:scale db))))
