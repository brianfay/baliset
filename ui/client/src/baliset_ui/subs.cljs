(ns baliset-ui.subs
  (:require [re-frame.core :as rf]))

(rf/reg-sub
 :node-metadata
 (fn [db _]
   (:node-metadata db)))
